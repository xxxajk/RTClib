/*
 * This is a port of a port of Dean Camera's ATOMIC_BLOCK macros for AVR 
 * to ARM Cortex M3 v1.0 to PIC32
 *
 */

#ifndef PIC32_ATOMIC_H
#define	PIC32_ATOMIC_H

static __inline__ uint32_t __get_gimask(void) {
        uint32_t gimask = disableInterrupts();
        return gimask;
}

static __inline__ void __set_gimask(uint32_t setval) {
        restoreInterrupts(setval);
        __asm__ volatile ("" :: : "memory");
}

static __inline__ uint32_t __iSeiRetVal(void) {
        asm volatile("ei");
        __asm__ volatile ("" :: : "memory");
        return 1;
}

static __inline__ uint32_t __iCliRetVal(void) {
        asm volatile("di");
        __asm__ volatile ("" :: : "memory");
        return 1;
}

static __inline__ void __iSeiParam(const uint32_t *__s) {
        asm volatile("ei");
        __asm__ volatile ("" :: : "memory");
        (void)__s;
}

static __inline__ void __iCliParam(const uint32_t *__s) {
        asm volatile("di");
        __asm__ volatile ("" :: : "memory");
        (void)__s;
}

static __inline__ void __iRestore(const uint32_t *__s) {
        __set_gimask(*__s);
}

#define ATOMIC_BLOCK(type) \
for ( type, __ToDo = __iCliRetVal(); __ToDo ; __ToDo = 0 )

#define ATOMIC_RESTORESTATE \
uint32_t gimask_save __attribute__((__cleanup__(__iRestore)))  = __get_gimask()

#define ATOMIC_FORCEON \
uint32_t gimask_save __attribute__((__cleanup__(__iSeiParam))) = 0

#define NONATOMIC_BLOCK(type) \
for ( type, __ToDo = __iSeiRetVal(); __ToDo ;  __ToDo = 0 )

#define NONATOMIC_RESTORESTATE \
uint32_t gimask_save __attribute__((__cleanup__(__iRestore))) = __get_gimask()

#define NONATOMIC_FORCEOFF \
uint32_t gimask_save __attribute__((__cleanup__(__iCliParam))) = 0


#endif	/* PIC32_ATOMIC_H */

