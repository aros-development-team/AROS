
struct a
{
	int x;
	int y;
};

int main(void)
{
	int z = (int)&(((struct a *)0x0) -> y);
}
