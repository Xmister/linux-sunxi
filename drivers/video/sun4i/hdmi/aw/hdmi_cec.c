#include "hdmi_cec.h"
#include "../hdmi_hal.h"
#include "hdmi_interface.h"
#include "hdmi_core.h"


static unsigned long long  _startTime = 0;
static unsigned long long  _endTime = 0;





extern __s32            hdmi_state;
extern __bool           video_enable;
extern __s32            video_mode;
extern HDMI_AUDIO_INFO  audio_info;

__bool          cec_enable;
__bool          cec_standby=0;
__u32            cec_phy_addr;
__u32            cec_logical_addr;
__u8                cec_count=30;

unsigned long long Hdmi_get_cur_time()//   1/24us
{
    TIME_LATCH();
    _startTime = (((unsigned long long)readl(_TIME_HIGH))<<32) | readl(_TIME_LOW);

    return _startTime;
}

//return us
__s32 Hdmi_time_diff(unsigned long long time_before, unsigned long long time_after)
{
    __u32 time_diff = (__u32)(time_after - time_before);
    return (__u32)(time_diff/24);
}

__s32 Hdmi_start_timer()
{
    TIME_LATCH();
    _startTime = (((unsigned long long)readl(_TIME_HIGH))<<32) | readl(_TIME_LOW);

    return 0;
}
    
__u32 Hdmi_calc_time() //us
{
    __u32 interval;
    
    TIME_LATCH();
    _endTime = (((unsigned long long)readl(_TIME_HIGH))<<32) | readl(_TIME_LOW);
    interval = (__u32)(_endTime - _startTime);

    
    return interval / 24;
    
} 

void Hdmi_DelayUS(__u32 Microsecond)
{
#if 0
    __u32 cnt = 1;
    __u32 delay_unit = 0;
    __u32 CoreClk = 120;

    delay_unit = (CoreClk * 1000 * 1000) / (1000*1000);
    cnt = Microsecond * delay_unit;
    while(cnt--);
#else
    unsigned long long t0, t1;
    //__inf("==delay %d\n", Microsecond);
    t1 = t0 = Hdmi_get_cur_time();
    while(Hdmi_time_diff(t0, t1) < Microsecond)
    {
        t1 = Hdmi_get_cur_time();
    }
    //__inf("t0=%d,%d\n",(__u32)(t0>>32), (__u32)t0);
    //__inf("t1=%d,%d\n",(__u32)(t1>>32), (__u32)t1);

    return ;
#endif
}

__s32 Hdmi_cec_enable(__bool en)
{
    if(en)
    {
        HDMI_WUINT32(0x214,HDMI_RUINT32(0x214) | 0x800);
    }else
    {
        HDMI_WUINT32(0x214,HDMI_RUINT32(0x214) & (~0x800));
    }

    cec_enable = en;

    return 0;
}

__s32 Hdmi_cec_write_reg_bit(__u32 data)//1bit
{
    if(data & 0x1)
    {
        HDMI_WUINT32(0x214,HDMI_RUINT32(0x214) | 0x200);
    }else
    {
        HDMI_WUINT32(0x214,HDMI_RUINT32(0x214) & (~0x200));
    }
    return 0;
}


__s32 Hdmi_cec_read_reg_bit()//1bit
{
    return ((HDMI_RUINT32(0x214) >> 8) & 0x1);
}

__s32 Hdmi_cec_start_bit()
{
    __u32 low_time, whole_time;
    unsigned long long t0,t1,t2;

    low_time = HDMI_CEC_START_BIT_LOW_TIME;
    whole_time = HDMI_CEC_START_BIT_WHOLE_TIME;

    //__inf("Hdmi_hal_cec_start_bit ===\n");
    
    Hdmi_cec_write_reg_bit(0);

    Hdmi_DelayUS(low_time);
    
    Hdmi_cec_write_reg_bit(1);

    Hdmi_DelayUS(whole_time-low_time);

    //__inf("start bit end\n");
    
    return 0;
}

__s32 Hdmi_cec_send_bit(__u32 data)//1bit
{
    __u32 low_time, whole_time;

    //__inf("Hdmi_hal_cec_send_bit===\n");

    low_time = (data==1)? HDMI_CEC_DATA_BIT1_LOW_TIME:HDMI_CEC_DATA_BIT0_LOW_TIIME;
    whole_time = HDMI_CEC_DATA_BIT_WHOLE_TIME;
    
    Hdmi_cec_write_reg_bit(0);

    Hdmi_DelayUS(low_time);
    
    Hdmi_cec_write_reg_bit(1);

    Hdmi_DelayUS(whole_time-low_time);

    //__inf("one bit over\n");
    
    return 0;
    
}

__s32 Hdmi_cec_ack(__u32 data)//1bit
{
    __u32 low_time, whole_time;

    //__inf("Hdmi_hal_cec_send_bit===\n");

    low_time = (data==1)? HDMI_CEC_DATA_BIT1_LOW_TIME:HDMI_CEC_DATA_BIT0_LOW_TIIME;
    whole_time = HDMI_CEC_DATA_BIT_WHOLE_TIME;
    
    while(Hdmi_cec_read_reg_bit() == 1);

    Hdmi_cec_write_reg_bit(0);

    Hdmi_DelayUS(low_time);
    
    Hdmi_cec_write_reg_bit(1);

    Hdmi_DelayUS(whole_time-low_time);

    //__inf("one bit over\n");
    
    return 0;
    
}


//active: 1: used while send msg; 
//          0: used while receive msg
__s32 Hdmi_cec_receive_bit(__bool active, __u32 *data)//1bit
{
    unsigned long long t0,t1,t2;
    __u32 low_level_time, hight_level_time;
    __s32 ret = 0;

    if(active)
    {
        Hdmi_cec_write_reg_bit(0);
        Hdmi_DelayUS(200);
        Hdmi_cec_write_reg_bit(1);
        Hdmi_DelayUS(800);
    }else
    {
        while(Hdmi_cec_read_reg_bit() == 1);
        Hdmi_DelayUS(1000);
    }
    *data = Hdmi_cec_read_reg_bit();

    if(active)
    {
        Hdmi_DelayUS(1400);
    }else
    {
        Hdmi_DelayUS(1100);
    }

    return ret;
    
}

__s32 Hdmi_cec_free()
{
    __inf("Hdmi_hal_cec_free!===\n");
    while(Hdmi_cec_read_reg_bit() != 0x1)
    {
        
    }
    __inf("loop out!===\n");

    Hdmi_start_timer();
    __inf("wait 7 data period===\n");
    while(Hdmi_cec_read_reg_bit() == 0x1)
    {
        if(Hdmi_calc_time() > 7*2400)
        {
            break;
        }
    }
    __inf("wait 7 data period end===\n");

    return 0;
}

__s32 Hdmi_cec_send_byte(__u32 data, __u32 eom, __u32 *ack)//1byte
{
    __u32 i;
    __s32 ret = 0;

    //__inf("Hdmi_hal_cec_send_byte!===\n");

    if(cec_enable == 0)
    {
        __inf("cec dont enable\n");
        return -1;
    }

    //__inf("data bit");
    for(i=0; i<8; i++)
    {
        if(data & 0x80)
        {
            Hdmi_cec_send_bit(1);
        }else
        {
            Hdmi_cec_send_bit(0);
        }
        data = data << 1;
    }

    //__inf("bit eom\n");
    Hdmi_cec_send_bit(eom);

    //Hdmi_hal_cec_send_bit(1);

    //todo?
    //__inf("receive ack\n");
    Hdmi_cec_receive_bit(1, ack);

    return 0;
}


__s32 hdmi_cec_receive_byte(__u32 *data, __u32 *eom)//1byte
{
    __u32 i;
    __u32 data_bit = 0;
    __u32 data_byte = 0;
    
    for(i=0; i<8; i++)
    {
        Hdmi_cec_receive_bit(0, &data_bit);
        data_byte = data_byte << 1;
        data_byte|= data_bit;
    }

    *data = data_byte;

    Hdmi_cec_receive_bit(0, eom);

    return 0;
}
__s32 Hdmi_cec_send_msg(__hdmi_cec_msg_t *msg)
{
    HDMI_BASE=0xf1c16000;

    __u32 header_block;
    __hdmi_cec_msg_eom eom;
    __u32 real_ack = 0;
    __u32 i;
    __u32 ack = (msg->follower_addr == HDMI_CEC_LADDR_BROADCAST)? HDMI_CEC_BROADCAST_MSG_ACK:HDMI_CEC_NORMAL_MSG_ACK;

    header_block = ((msg->initiator_addr&0xf) << 4) | (msg->follower_addr&0xf);
    eom = (msg->opcode_valid)? HDMI_CEC_MSG_MORE:HDMI_CEC_MSG_END;
    
    Hdmi_cec_enable(1);
    Hdmi_cec_start_bit();                          //start bit
    Hdmi_cec_send_byte(header_block, eom, &real_ack);   //header block

    if((real_ack == ack) && (msg->opcode_valid))
    {
        eom = (msg->para_num != 0)? HDMI_CEC_MSG_MORE:HDMI_CEC_MSG_END;
        Hdmi_cec_send_byte(msg->opcode, eom, &real_ack); //data block: opcode

        if(real_ack == ack)
        {
            for(i=0; i<msg->para_num; i++)
            {
                eom = (i == (msg->para_num-1))? HDMI_CEC_MSG_END:HDMI_CEC_MSG_MORE;
                Hdmi_cec_send_byte(msg->para[i], eom, &real_ack); //data block: parameters
            }
        }
    }
    Hdmi_cec_enable(0);

    __inf("ack:%d\n", real_ack);
    return real_ack;
}

__s32 Hdmi_cec_wait_for_start_bit()
{
    __u32 i;
    __s32 ret;

    __inf("wait for stbit\n");
    while(1)
    {
        while(Hdmi_cec_read_reg_bit() == 1);

        for(i=0; i<7; i++)
        {
            if(Hdmi_cec_read_reg_bit() == 1)
                break;   

            Hdmi_DelayUS(500);
        }

        if(i<7)continue;

        while(Hdmi_cec_read_reg_bit() == 0);

        for(i=0; i<4; i++)
        {
            if(Hdmi_cec_read_reg_bit() == 0)
                break;   

            Hdmi_DelayUS(100);
        }

        if(i<4)continue;
        else break;
    }

    return ret;
}

__s32 Hdmi_cec_receive_msg(__hdmi_cec_msg_t *msg)
{
    __s32 ret;
    __u32 data_bit;
    __u32 data_byte;
    __u32 ack;
    __u32 i;
    __u32 eom;
    cec_logical_addr=0x04;

    memset(msg, 0, sizeof(__hdmi_cec_msg_t));
    
    Hdmi_cec_wait_for_start_bit();

    hdmi_cec_receive_byte(&data_byte, &eom);

    msg->initiator_addr = (data_byte >> 4)&0x0f;
    msg->follower_addr = data_byte & 0x0f;

    if((msg->follower_addr == cec_logical_addr) || (msg->follower_addr ==  HDMI_CEC_LADDR_BROADCAST))
    {
        ack = (msg->follower_addr == cec_logical_addr)? 0:1;
        Hdmi_cec_ack(ack);

        if(!eom)
        {
            hdmi_cec_receive_byte(&data_byte, &eom);
            msg->opcode = data_byte;
            msg->opcode_valid = 1;
            
            Hdmi_cec_ack(ack);
            
            while(!eom)
            {
                hdmi_cec_receive_byte(&data_byte, &eom);
                msg->para[msg->para_num] = data_byte;
                msg->para_num ++;

                Hdmi_cec_ack(ack);
            }
        }
   
    printk("%d, %d\n", msg->initiator_addr, msg->follower_addr);
    }else
    {
        Hdmi_cec_ack(1);
    }

    if(msg->opcode_valid)
    {
        printk("op: 0x%x\n", msg->opcode);
    }

    for(i=0; i<msg->para_num; i++)
    {
        printk("para[%d]: 0x%x\n", i, msg->para[i]);
    }

    return 0;
}


__s32 Hdmi_cec_intercept(__hdmi_cec_msg_t *msg)
{
    __s32 ret;
    __u32 data_bit;
    __u32 data_byte;
    __u32 ack;
    __u32 i;
    __u32 eom;

    memset(msg, 0, sizeof(__hdmi_cec_msg_t));
    
    Hdmi_cec_wait_for_start_bit();

    hdmi_cec_receive_byte(&data_byte, &eom);

    msg->initiator_addr = (data_byte >> 4)&0x0f;
    msg->follower_addr = data_byte & 0x0f;

    Hdmi_cec_receive_bit(0, &data_bit);   //skip ack bit
    
    if(!eom)
    {    
        hdmi_cec_receive_byte(&data_byte, &eom);
        msg->opcode = data_byte;
        msg->opcode_valid = 1;

        Hdmi_cec_receive_bit(0, &data_bit); //skip ack bit
        while(!eom)
        {     
            hdmi_cec_receive_byte(&data_byte, &eom);
            msg->para[msg->para_num] = data_byte;
            msg->para_num ++;

            Hdmi_cec_receive_bit(0, &data_bit); //skip ack bit
        }
    }

    __inf("%d-->%d\n", msg->initiator_addr, msg->follower_addr);
    if(msg->opcode_valid)
    {
        __inf("op: %x\n", msg->opcode);
    }

    for(i=0; i<msg->para_num; i++)
    {
        __inf("para[%d]: %x\n", i, msg->para[i]);
    }
    
    return 0;
}


//__u32 cec_phy_addr = 0x1000;/
//__u32  cec_logical_addr = 0x4; //4bit
//return ack
__s32 Hdmi_cec_ping(__u32 init_addr, __u32 follower_addr)
{
    __hdmi_cec_msg_t msg;

    memset(&msg, 0, sizeof(__hdmi_cec_msg_t));
    msg.initiator_addr = init_addr;
    msg.follower_addr = follower_addr;
    msg.opcode_valid = 0;
    
    return Hdmi_cec_send_msg(&msg);
}



//cmd: 0xffffffff(no cmd)
__s32 Hdmi_cec_send_cmd(__u32 init_addr, __u32 follower_addr, __u32 cmd, __u32 para, __u32 para_bytes)
{
    __u32 header_of_msg = 0x44; //8bit
    __u32 end_of_msg = 0x1;// 1bit
    __u32 ack        = 0x0;//1bit
    __s32 ret;

    //broadcast msg
    if(follower_addr == 0xf)
    {
    }
    header_of_msg = ((init_addr&0xf) << 8) | (follower_addr&0xf);
    
    Hdmi_cec_enable(1);
    
    Hdmi_cec_start_bit();

    if(cmd == 0xffffffff)
    {
        Hdmi_cec_send_byte(header_of_msg, 1, &ack);
        if(ack == 1)
        {
            __inf("===Hdmi_cec_send_cmd, ok \n");
            ret = 0;
        }else
        {
            __inf("###Hdmi_cec_send_cmd, fail \n");
            ret = -1;
        }
    }else
    {
        Hdmi_cec_send_byte(header_of_msg, 0, &ack);
        if(ack == 0)
        {
            
        }else
        {
            
        }
    }

    Hdmi_cec_enable(0);

    return 0;
}


__s32 Hdmi_cec_test()
{
    __u32 header_of_msg = 0x40; //8bit
    __u32 end_of_msg = 0x1;// 1bit
    __u32 ack        = 0x1;//1bit
    __u32 cmd = 0x04;
    __u32 i;
    HDMI_BASE=0xf1c16000;
    printk("###########################Hdmi_cec_test\n");
    //__u32 data = 0x00036;  // 0d
    //__u32 data = 0x00013; //04
    __u32 data = 0x40013; //04
#if 0
    __inf("===enable\n");
    Hdmi_hal_cec_enable(1);

    __inf("===start bit\n");
    Hdmi_hal_cec_start_bit();


    for(i=0; i<20; i++)
    {
        if(data & 0x80000)
        {
            Hdmi_hal_cec_send_bit(1);
        }else
        {
            Hdmi_hal_cec_send_bit(0);
        }
        data = data << 1;
    }
#endif

#if 1
/*
__inf("===enable\n");
    Hdmi_hal_cec_enable(1);

    __inf("===start bit\n");
    Hdmi_cec_start_bit();
    
    Hdmi_cec_send_byte(0x40, 0, &ack);

    Hdmi_cec_send_byte(0x04, 1,&ack);
*/
    __hdmi_cec_msg_t msg;
#if 1   
    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_TV;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_IMAGE_VIEW_ON;
    msg.para_num = 0;
    Hdmi_cec_send_msg(&msg);

    Hdmi_DelayUS(100000);
    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_TV;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_IMAGE_VIEW_ON;
    msg.para_num = 0;
    Hdmi_cec_send_msg(&msg);
    

    Hdmi_DelayUS(100000);
    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_TV;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_ACTIVE_SOURCE;
    msg.para_num = 2;
    msg.para[0] = (cec_phy_addr >> 8) & 0xff;
    msg.para[1] = cec_phy_addr & 0xff;
    Hdmi_cec_send_msg(&msg);

    Hdmi_DelayUS(100000);
    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_TV;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_ACTIVE_SOURCE;
    msg.para_num = 2;
    msg.para[0] = (cec_phy_addr >> 8) & 0xff;
    msg.para[1] = cec_phy_addr & 0xff;
    Hdmi_cec_send_msg(&msg);
/*
    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_BROADCAST;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_ACTIVE_SOURCE;
    msg.para_num = 2;
    msg.para[0] = 0x20;
    msg.para[1] = 0x00;
    Hdmi_hal_cec_send_msg(&msg);


    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_TV;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_SET_OSD_NAME;
    msg.para_num = 9;
    msg.para[0] = 'A';
    msg.para[1] = 'L';
    msg.para[2] = 'L';
    msg.para[3] = 'W';
    msg.para[4] = 'I';
    msg.para[5] = 'N';
    msg.para[6] = 'N';
    msg.para[7] = 'E';
    msg.para[8] = 'R';
    Hdmi_hal_cec_send_msg(&msg);


    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_TV;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_SET_OSD_NAME;
    msg.para_num = 9;
    msg.para[0] = 'A';
    msg.para[1] = 'L';
    msg.para[2] = 'L';
    msg.para[3] = 'W';
    msg.para[4] = 'I';
    msg.para[5] = 'N';
    msg.para[6] = 'N';
    msg.para[7] = 'E';
    msg.para[8] = 'R';
    Hdmi_hal_cec_send_msg(&msg);


    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_TV;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_SET_OSD_NAME;
    msg.para_num = 9;
    msg.para[0] = 'A';
    msg.para[1] = 'L';
    msg.para[2] = 'L';
    msg.para[3] = 'W';
    msg.para[4] = 'I';
    msg.para[5] = 'N';
    msg.para[6] = 'N';
    msg.para[7] = 'E';
    msg.para[8] = 'R';
    Hdmi_hal_cec_send_msg(&msg);

*/
    //Hdmi_hal_DelayUS(4000000);

#endif
   

    Hdmi_DelayUS(100000);
    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
    msg.follower_addr = HDMI_CEC_LADDR_TV;
    msg.opcode_valid = 1;
    msg.opcode = HDMI_CEC_OP_REQUEST_POWER_STATUS;
    msg.para_num = 0;
    Hdmi_cec_send_msg(&msg);

    Hdmi_DelayUS(100000);


    
    
    //while(!Hdmi_hal_cec_intercept(&msg));
/*    i = 0;
    while(!Hdmi_hal_cec_receive_msg(&msg))
    {
        i++;
        if(i == 30)
        {
            Hdmi_hal_DelayUS(100000);
            msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
            msg.follower_addr = HDMI_CEC_LADDR_BROADCAST;
            msg.opcode_valid = 1;
            msg.opcode = HDMI_CEC_OP_REQUEST_POWER_STATUS;
            msg.para_num = 0;
            Hdmi_hal_cec_send_msg(&msg);
            __inf("==get pwr st\n");

            i = 0;
        }
    }
*/
    

#endif
#if 0
    __inf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    __inf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    __inf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    Hdmi_hal_cec_start_bit();
    data = 0x103;
    for(i=0; i<10; i++)
    {
      if(data & 0x200)
      {
          Hdmi_hal_cec_send_bit(1);
      }else
      {
          Hdmi_hal_cec_send_bit(0);
      }
      data = data << 1;
    }
#endif

    __inf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    __inf("++++++++++++++++  active source +++++++++++++++++++++++++++++\n");
    __inf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

//    data = 0x0f609204;  2切成功
    //data = 0x0f609404; //4切成功
    data = 0x4f609104;
    __inf("===start bit\n");
    Hdmi_cec_start_bit();

    for(i=0; i<32; i++)
    {
        if(data & 0x80000000)
        {
            Hdmi_cec_send_bit(1);
        }else
        {
            Hdmi_cec_send_bit(0);
        }
        data = data << 1;
    }

        data = 0x03;
        for(i=0; i<8; i++)
        {
            if(data & 0x80)
            {
                Hdmi_cec_send_bit(1);
            }else
            {
                Hdmi_cec_send_bit(0);
            }
            data = data << 1;
        }


    __inf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    __inf("++++++++++++++++  active source +++++++++++++++++++++++++++++\n");
    __inf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

//    data = 0x0f609204;  2切成功
    //data = 0x0f609404; //4切成功
    data = 0x4f609104;
    __inf("===start bit\n");
    Hdmi_cec_start_bit();

    for(i=0; i<32; i++)
    {
        if(data & 0x80000000)
        {
            Hdmi_cec_send_bit(1);
        }else
        {
            Hdmi_cec_send_bit(0);
        }
        data = data << 1;
    }

        data = 0x03;
        for(i=0; i<8; i++)
        {
            if(data & 0x80)
            {
                Hdmi_cec_send_bit(1);
            }else
            {
                Hdmi_cec_send_bit(0);
            }
            data = data << 1;
        }


    Hdmi_cec_enable(0);

    return 0;
    
}

__s32 Hdmi_cec_init()
{
    cec_enable = 0;
    cec_logical_addr = HDMI_CEC_LADDR_PAYER1;
}


void hdmi_cec_task_loop()
{

        __hdmi_cec_msg_t  msg;
    if(!cec_standby)
        {
        switch(cec_count%10)
            {
                case 9:
                    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
                    msg.follower_addr = HDMI_CEC_LADDR_TV;
                    msg.opcode_valid = 1;
                    msg.opcode = HDMI_CEC_OP_IMAGE_VIEW_ON;
                    msg.para_num = 0;
        
                    Hdmi_cec_send_msg(&msg);
                    printk("#########################HDMI_CEC_OP_IMAGE_VIEW_ON\n");
                    break;
                case 7:
                    msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
                    msg.follower_addr = HDMI_CEC_LADDR_BROADCAST;
                    msg.opcode_valid = 1;
                    msg.opcode = HDMI_CEC_OP_ACTIVE_SOURCE;
                    msg.para_num = 2;
                    msg.para[0] = (cec_phy_addr >> 8) & 0xff;
                    msg.para[1] = cec_phy_addr & 0xff;
  
                    Hdmi_cec_send_msg(&msg);

                     printk("#########################HDMI_CEC_LADDR_BROADCAST\n");
                    break;
                default :
                   
                    break;
                
            }
        }
    else
        {
                switch(cec_count%10)
                    {
                    	
                    		case 9:
                        		msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
                        		msg.follower_addr = HDMI_CEC_LADDR_TV;
                        		msg.opcode_valid = 1;
                        		msg.opcode = HDMI_CEC_OP_INACTIVE_SOURCE;
                        		msg.para_num = 2;
                    				msg.para[0] = (cec_phy_addr >> 8) & 0xff;
                    				msg.para[1] = cec_phy_addr & 0xff;         
                        		Hdmi_cec_send_msg(&msg);
                        case 7:
                        		msg.initiator_addr = HDMI_CEC_LADDR_PAYER1;
                        		msg.follower_addr = HDMI_CEC_LADDR_BROADCAST;
                        		msg.opcode_valid = 1;
                        		msg.opcode = HDMI_CEC_OP_STANDBY;
                        		msg.para_num = 0;
         
                        		Hdmi_cec_send_msg(&msg);
                    }
        }
    if(cec_count>1)cec_count--;
}

