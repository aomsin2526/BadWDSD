FUNC_DEF void dead_beep()
{
    WaitInMs(2000);

    sc_continuous_beep();
    dead();
}