struct SignalTranslation
{
    short sig;
    short AmigaTrap;
    short CPUTrap;
};

extern struct SignalTranslation sigs[];

extern sigset_t sig_int_mask;
extern unsigned int supervisor;
