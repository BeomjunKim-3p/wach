#ifndef WA824I_MAC_H
#define WA824I_MAC_H

/*
 * 서로 다른 블럭(-----)에 있는 매크로는 함께 쓰일 수 있다. 
 * */


/* --------------------------------------------------------------------------------------- */


/*
 * INOUT_, OUT_, TAKE_가 없으면 포인터를 const로 선언해야 합니다.
 * TAKE_ const는 "use after free" 가능성 때문에 불가합니다. 
 * */

/*
 * 포인터가 가리키는 객체의 ownership을 callee에게 이전합니다.
 * 호출 이후 caller는 해당 포인터를 통해 객체에 접근해서는 안 됩니다.
 *
 * TAKE_는 ALLOC_ 또는 STATIC_과 함께 쓰여야 합니다.
 *
 * ex) static 객체를 전달하고 callee가 포인터를 저장하여 독자적으로 사용한다.
 * ex) allocated 객체를 전달하고 callee가 해당 객체의 lifetime을 관리한다.
 */
#define TAKE_ /* TAKE_ */

/*
 * 이전 값을 읽지 않습니다. 쓰기만 합니다.
 * */
#define OUT_ /* OUT */

/* 
 * 읽고, 씁니다.
 * */
#define INOUT_ /* INOUT */

/* ---------------------------------------------------------------------------------------- */ 

/*
 * NULL을 넘길 수 있습니다.
 * */
#define OPT_ /* OPT */

/* ---------------------------------------------------------------------------------------- */ 

/*
 * SDA(storage duration annotation)가 없다면 
 * 모든 SDA 객체를 전달할 수 있습니다.
 *
 * 여러개의 SDA를 함께 사용할 경우 OR로 연결됩니다.
 * 즉  STATIC_ ALLOC_ 은 static 또는 allocated storage duration을 허용합니다. 
 * */

/*
 * Allocated storage duration 객체만 넘길 수 있다는 뜻.
 * */
#define ALLOC_ /* ALLOC_ */

/* 
 * Static storage duration 객체만 넘길 수 있다는 뜻.
 * */
#define STATIC_ /* STATIC_ */


#endif


