asm(".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,@function\n"
	"start: ldr	%r12, 1f\n\t"
	"		ldr	pc, [%r12]\n\t"
	"		.word startup"
)

void startup()
{
	while(1);
}
