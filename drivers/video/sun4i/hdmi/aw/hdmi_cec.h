//#include "../hdmi_hal.h"
#include "hdmi_interface.h"
//#include "hdmi_core.h"

//For 64 bits high-precision timer ===
#define _TIME_LOW   (0xf1c20c00 + 0xa4)
#define _TIME_HIGH  (0xf1c20c00 + 0xa8)
#define _TIME_CTL   (0xf1c20c00 + 0xa0)
#define TIME_LATCH()  (*(volatile __u32 *)(_TIME_CTL )|= ((__u32)0x1)<<1)


//time us
#define HDMI_CEC_START_BIT_LOW_TIME 3700
#define HDMI_CEC_START_BIT_WHOLE_TIME 4500

#define HDMI_CEC_DATA_BIT0_LOW_TIIME 1500          
#define HDMI_CEC_DATA_BIT1_LOW_TIME  600
#define HDMI_CEC_DATA_BIT_WHOLE_TIME 2400
//ack
#define HDMI_CEC_NORMAL_MSG_ACK 0X0
#define HDMI_CEC_BROADCAST_MSG_ACK 0X1

typedef enum
{
    HDMI_CEC_OP_IMAGE_VIEW_ON                  = 0X04,
    HDMI_CEC_OP_TEXT_VIEW_ON                   = 0X0D,  
    HDMI_CEC_OP_STANDBY                        = 0X36,
    HDMI_CEC_OP_SET_OSD_NAME                   = 0X47,
    HDMI_CEC_OP_ROUTING_CHANGE                 = 0X80,
    HDMI_CEC_OP_ACTIVE_SOURCE                  = 0X82,
    HDMI_CEC_OP_REPORT_PHY_ADDRESS             = 0X84,
    HDMI_CEC_OP_DEVICE_VENDOR_ID               = 0X87,
    HDMI_CEC_OP_MENU_STATE                     = 0X8E,
    HDMI_CEC_OP_REQUEST_POWER_STATUS           = 0X8F,
    HDMI_CEC_OP_REPORT_POWER_STATUS            = 0X90,
    HDMI_CEC_OP_INACTIVE_SOURCE                = 0X9D,
    HDMI_CEC_OP_NUM                            = 0xff,
}__hdmi_cec_opcode;

typedef enum
{
    HDMI_CEC_LADDR_TV,
    HDMI_CEC_LADDR_RECORDER1,
    HDMI_CEC_LADDR_RECORDER2,
    HDMI_CEC_LADDR_TUNER1,
    HDMI_CEC_LADDR_PAYER1,
    HDMI_CEC_LADDR_AUDIO,
    HDMI_CEC_LADDR_TUNER2,
    HDMI_CEC_LADDR_TUNER3,
    HDMI_CEC_LADDR_PAYER2,
    HDMI_CEC_LADDR_RECORDER3,
    HDMI_CEC_LADDR_TUNER4,
    HDMI_CEC_LADDR_PAYER3,
    HDMI_CEC_LADDR_RESERVED1,
    HDMI_CEC_LADDR_RESERVED2,
    HDMI_CEC_LADDR_SPECIFIC,
    HDMI_CEC_LADDR_BROADCAST,
}__hdmi_cec_logical_address;

typedef enum
{
    HDMI_CEC_MSG_MORE,
    HDMI_CEC_MSG_END,
}__hdmi_cec_msg_eom;

typedef struct
{
    __hdmi_cec_logical_address initiator_addr;
    __hdmi_cec_logical_address follower_addr; 
    __u32 opcode_valid; //indicate there is opcode or not
    __hdmi_cec_opcode opcode;
    __u32 para[14];   //byte
    __u32 para_num;   //byte <16byte
}__hdmi_cec_msg_t;

extern __bool          cec_enable;
extern __bool       cec_standby;
extern __u32            cec_phy_addr;
extern __u32            cec_logical_addr;
extern __u8            cec_count;


extern __s32 Hdmi_cec_test();
extern __s32 Hdmi_cec_send_msg(__hdmi_cec_msg_t *msg);
extern __s32 Hdmi_cec_receive_msg(__hdmi_cec_msg_t *msg);
extern void hdmi_cec_task_loop();