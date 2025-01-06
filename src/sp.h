/*
 * sp.h
 *
 *  Created on: Sep 17, 2024
 *      Author: debin
 */

#ifndef SP_H_
#define SP_H_

extern void setPSP(uint32_t* p);
extern uint32_t* getPSP(void);
extern void setMSP(void);
extern uint32_t* getMSP(void);
extern void setASP(void);
extern void setTMPL(void);
extern void MPUFaultCause(void);
extern uint32_t getSVCnum(void);
extern void popREGS(void);
extern void pushREGS(void);
extern uint32_t ReadFromR1(void);
#endif /* SP_H_ */
