-- test08.lua 

require("SimCore")
MemsInsertRAM(0x0800,0x0800)
MemsInsertRAM(0x2000,0x0400)
MemsInsertROM(0xC000,0x4000)
MemsConfig()
MemsReportList()
MemsReadS19("ex08.s19")
MemsDumpRange(0xC000,0x0020)
MemsDumpRange(0xFFF0,0x0010)

pc = CPU_Reset()
for ix = 1,90 do
  CPU_ReportRegs()
  CPU_DisOpCode(pc)
  pc = CPU_FetchEx()
end
CPU_ReportRegs()
MemsDumpRange(0x0800,0x0010)
