struct SignalTranslation
{
    short sig;
    short AmigaTrap;
    short CPUTrap;
};

extern struct SignalTranslation sigs[];
