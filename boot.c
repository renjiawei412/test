#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "main.h"
          
enum
{
	_STA_NOT_LINKED,
	_STA_LINKED    
} _E_State;

#define  _BOOT_REQ      0xFF
#define  _NO_BOOT_REQ   0x00
#define  _ERROR_CANRX   0x11
#define  _ERROR_ID      0x22 
static u8 _Boot_Flag;

typedef struct 
{
    unsigned int id;
    Bool RTR;
    unsigned char data[8];
    unsigned char len;
    unsigned char prty;
} CAN0_DATATYPE rxMsg;
static CAN0_DATATYPE msgRX;

#pragma CODE_SEG APP_ROM_ENTRY
void _EntryPoint(void)
{
	 __asm(ldx   #$C000);   
	 __asm(jmp    0,x);
} 

void _GoBoot(void)
{
	__asm(lds  #$3900);
	__asm(jmp  _Startup);
}
#pragma CODE_SEG  DEFAULT

Bool MSCANGetMsg(struct can_msg *msg)
{
	unsigned char sp2;
	
	// 检测接收标志
	if(!(CANRFLG_RXF))
		return(FALSE);
	
	// 检测 CAN协议报文模式 （一般/扩展） 标识符
	if(CANRXIDR1_IDE)
		// IDE = Recessive (Extended Mode)
		return(FALSE);
	
	// 读标识符
	msg->id = (unsigned int)(CANRXIDR0<<3) | 
				(unsigned char)(CANRXIDR1>>5);
	
	if(CANRXIDR1&0x10)
		msg->RTR = TRUE;
	else
		msg->RTR = FALSE;
	
	// 读取数据长度 
	msg->len = CANRXDLR;
	
	// 读取数据
	for(sp2 = 0; sp2 < msg->len; sp2++)
		msg->data[sp2] = *((&CANRXDSR0)+sp2);
	
	// 清 RXF 标志位 (缓冲器准备接收)
	CANRFLG = 0x01;
	
	return(TRUE);
}

u8 CanRxRead(CAN0_DATATYPE * rxMsg)
{
	if(TRUE == MSCANGetMsg(rxMsg)) /*only Standard format ID*/
	{
		/*judeg if it is bootrequest: 0xAA ... */
		
		if(DownLoadID == rxMsg.id)
		{
			if(0xAA==rxMsg.data[0] && 0xAA==rxMsg.data[1] && 0xAA==rxMsg.data[2])
			{
				return _BOOT_REQ;
			}
		}
		else
			return _ERROR_ID;
	}
	else
	{
		return _ERROR_CANRX
		/*return error code*/
	}

}

void CAN0_init_boot(void)
{
  if(CANCTL0_INITRQ==0)      // 查询是否进入初始化状态   
    CANCTL0_INITRQ =1;        // 进入初始化状态
  while (CANCTL1_INITAK==0);  //等待进入初始化状态
  
  CANBTR0_BRP = 3;                    //Baud Rate Prescaler
  CANBTR0_SJW = 0;                    //SJW = (SJW+1)*Tq = Tq
  CANBTR1 = 0x14;             //设置时段1和时段2的Tq个数 ,总线频率为0x1c->250kb/s ,0x14->500k
  
  CANIDAC_IDAM0 = 0;                  //00:2*32bit 01:4*16bit  
  CANIDAC_IDAM1 = 0;                  //10:8*8bit  11:closed,no message is acceped
  
  CANIDMR0 = 0XFF;   
  CANIDMR1 = 0XFF;      
  CANIDMR2 = 0XFF;          
  CANIDMR3 = 0XFF;          
  CANIDMR4 = 0XFF;   
  CANIDMR5 = 0XFF;  
  CANIDMR6 = 0XFF;                
  CANIDMR7 = 0XFF;
  
  CANCTL1 = 0xC0;             //使能MSCAN模块,设置为一般运行模式、使用总线时钟源 
  CANCTL0 = 0x00;             //返回一般模式运行
  CANCTL0_INITRQ = 0;
  while (CANCTL1_INITAK==1) ;   
  while (CANCTL0_SYNCH==0) ;
  CANRIER = 0;
  CANTIER = 0;   

}

void Dlyms(int tick) 
{          
    int i,j,k=0;
    
    for (i=0;i<tick;i++) 
    {
        for (j=0;j<300;j++) 
        {
            k++;
        }
    }
}

#pragma CODE_SEG  BOOT_SEG

void bootStart(void) 
{
	/* put your own code here */
	_Boot_Flag = _NO_BOOT_REQ;
    _E_State = _STA_NOT_LINKED;
	
	DisableInterrupts;
    SetBusCLK_M(16); 
//	CopyCodeToRAM();
	InterruptModuleSetup();
//	TIM_Init(); 
//	FlashInit();
	
	CAN0_init_boot();   

	EnableInterrupts;

	// wait 500ms for Bootload link request from host
	for (i=0; i<1000; i++) 
	{   
		_Boot_Flag = CanRxRead(&rxMsg);//读取寄存器CAN数据，并进行判断,有boot请求，return 1

		if (_Boot_Flag == _BOOT_REQ)
		{
			_Boot_Flag = _NO_BOOT_REQ;
			_E_State = _STA_LINKED;	 /*没用上*/	
			_Goboot();		
			break;
		}
		else 
		{
			Dlyms(1);
		}
	}
	for(;;) 
	{
		_EntryPoint();
	} /* loop forever */
	/* please make sure that you never leave main */
}
#pragma CODE_SEG  DEFAULT




