-- --------------------------------------------------------------------
-- HwSim6809cli.lua  - Part of HwSim6809 project
-- This module starts the system, then calls CmdFile
-- Krista Hill - kmhill@hartford.edu - 01/15/2013 - version 0.1.0
-- Krista Hill - kmhill@hartford.edu - 12/31/2012 - File is created
-- --------------------------------------------------------------------

-- Pull in other files
require("SimCore")
require("CmdFile")

-- Obtain a file name
if arg[1] then
  FileName = arg[1]
else
  io.write("S19 file to open: ")
  FileName = io.read()
end

-- Configure the memory system
MemsInsertRAM(0x0800,0x0800)
MemsInsertROM(0xC000,0x4000)
MemsConfig()
MemsReportList()

-- Read the S19 file and reset
io.write("Opening '",FileName,"'\n")
MemsReadS19(FileName)
io.write("Resetting the CPU\n")
CPU_Reset()

-- Enter the command environment
CmdTop("HwSim6809 ver: 0.1.0")

print("Exiting")
-- end of SysCore.lua
