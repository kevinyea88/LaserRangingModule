namespace SGService.LaserRangingModule.Monitor
{
    // immutable

    internal record LrmParameters
    {
        public int Address { get; init; } = 0x80;

        public LrmRange Range { get; init; } = LrmRange.FiveMeter;

        public LrmResolution Resolution { get; init; } = LrmResolution.OneHundredMicormeter;

        public LrmFrequency Frequency { get; init; } = LrmFrequency.FiveHerz;

        public LrmStartPosition StartPosition { get; init; } = LrmStartPosition.Tail;
    }
}
