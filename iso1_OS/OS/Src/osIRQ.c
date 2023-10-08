#include "osIRQ.h"
#include "osKernel.h"

extern osIRQVector irqVector[IRQ_NUMBER];

bool osRegisterIRQ(osIRQnType irqType, IRQHandler function, void *data)
{

	/* osIRQnType is the ENUM for the IRQ Handler from 0 - 91 */
	/* for example ADC_IRQHandler = 18 */

	/* First we check if the irqType is between 0 and 91 */
	if (irqType > 91 || irqType < 0) return false;

	/* Now check that the function handler is not NULL */
	if (function == NULL) return false;

	/* If something is inside the vector return */
	if (irqVector[irqType].handler != NULL) return false;

	/* IRQHandler function is the function that is going to be executed when the IRQ is called */
	/* Data is the data that you can pass to the handler. Could be NULL */

	osIRQVector v;
	v.handler = function;
	v.data = data;

	/* If we have success with the irqType and function we take the irqVector and put the handler in the correct place */
	irqVector[irqType] = v;

	NVIC_ClearPendingIRQ(irqType);
	NVIC_EnableIRQ(irqType);
	return true;

}

bool osUnregisterIRQ(osIRQnType irqType)
{
	/* First we check if the irqType is beetween 0 and 91 */
	if (irqType > 91 || irqType < 0) return false;

	osIRQVector v;
	v.handler = NULL;
	v.data = NULL;

	irqVector[irqType] = v;

	NVIC_ClearPendingIRQ(irqType);
	NVIC_DisableIRQ(irqType);

	return true;
}

