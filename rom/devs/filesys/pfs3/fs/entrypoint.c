/* Entrypoint of the filesystem */
void __saveds EntryPoint(void);
int entrypoint(void)
{
	/* NG systems (at least MorphOS) manages without SwapSwap - so just enter the FS */
	EntryPoint();
	return 0;
}
