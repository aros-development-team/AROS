# supervisor macros

#define SUPER				  \
	cmpb	$0,supervisor		; \
	jne	1f			; \
	movl	%esp,usp		; \
	movl	ssp,%esp		; \
	movb	$1,supervisor		; \
1:

#define USER				  \
	cmpb	$0,supervisor		; \
	je	1f			; \
	movl	%esp,ssp		; \
	movl	usp,%esp		; \
	movb	$0,supervisor		; \
1:

#define REG_STORE			  \
	pushal				; \
	movl	%esp,esp

#define REG_RESTR			  \
	movl	esp,%esp		; \
	popal

