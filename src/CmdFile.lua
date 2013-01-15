-- --------------------------------------------------------------------
-- CmdFile.lua - Part of HwSim6809 project
-- This module provides a simple command line interface, and is
-- called by HwSim6809cli.lua, which prepares system startup
-- Krista Hill - kmhill@hartford.edu - 01/15/2013 - version 0.1.0
-- Krista Hill - kmhill@hartford.edu - 12/31/2012 - File is created
-- --------------------------------------------------------------------
DumpAddx = 0x0
DumpSize = 0x10
ListAddx = 0x0
ListSize = 1

function CmdTop(msg)
  print(msg)
  print("Enter 'h' for the help menu")
  pc = CPU_GetPCreg()
  DumpAddx = pc
  ListAddx = pc
  CPU_ReportRegs()
  CPU_DisOpCode(pc)
  
  
  while(true) do
    --MemsDumpRange(0xFFF0,0x10)
    -- Issue prompt and get the user input line
    io.write('> ')
    line = io.read('*line')
    
    -- Determine which command is entered
    a,b,c = string.find(line,'(%a+)')
    if line ~= '' then 
      if a== nil then print("?? - use 'h' for help")
      elseif c == 'c' then CPU_ReportRegs()
      elseif c == 'h' then HelpMenu()
      elseif c == 'l' then CmdList(line,b)
      elseif c == 'r' then CmdReset()
      elseif c == 's' then CmdStep()
      elseif c == 'd' then CmdDump(line,b)
      elseif c == 'q' or c == 'quit' or c == 'x' then break;
      else io.write("?? '",line,"'\n")
      end
    end
  end
end

------------------------------------------------------------------
function HelpMenu()
-- Produce a list of the commands
------------------------------------------------------------------
  print("Help Menu:")
  print(
[=['h' - produce this output, 'q' or 'x' - exit
'c - report CPU register values,'
'd [start_addx [num_bytes]]' - dump memory,
'l [[start_addx] num_inst]' - list instructions
'r' - reset the processor,
's [no_instructions]' - step through instructions]=])
end

------------------------------------------------------------------
function CmdDump(line,b)
-- Produce a memory dump
------------------------------------------------------------------

  -- Look for a new dump address
  a,b,s1 = string.find(line,'$?(%x+)',b+1)
  if a~= nil then
    DumpAddx = tonumber('0x'..s1)

    -- Look for a new dump size
    a,b,s2 = string.find(line,'$?(%x+)',b+1)
    if a~= nil then
      DumpSize = tonumber('0x'..s2)
    end
  end
  
  -- Produce the memory dump
  MemsDumpRange(DumpAddx,DumpSize)
  DumpAddx = DumpAddx + DumpSize
end

------------------------------------------------------------------
function CmdList(line,b)
-- list a few instructions by disassembling memory
------------------------------------------------------------------

  -- Look for a value
  a,b,s1 = string.find(line,'$?(%x+)',b+1)
  if a~= nil then
    local v1 = tonumber('0x'..s1)
  
    -- Look for another value
    a,b,s2 = string.find(line,'$?(%x+)',b+1)
    if a == nil then
      ListSize = tonumber('0x'..s1)
    else
      ListAddx = tonumber('0x'..s1)
      ListSize = tonumber('0x'..s2)
    end
  end
  
  -- Output instructions
  local addx = ListAddx
  local nn = ListSize
  while (nn > 0) do
    io.write(string.format("%.4X: ",addx))
    addx = CPU_DisOpCode(addx)
    if addx > 0xFFFF then break end
    nn = nn - 1
  end
  if addx <= 0xFFFF then 
    ListAddx = addx
  end
end

------------------------------------------------------------------
function CmdReset()
-- Reset the processor core
------------------------------------------------------------------
  print('Resetting the CPU')
  CPU_Reset()
  CPU_ReportRegs()
end

------------------------------------------------------------------
function CmdStep()
-- Execute a given number of instructions
------------------------------------------------------------------
  local StepInst = 1

  -- Look for a number of instructions
  a,b,s1 = string.find(line,'$?(%x+)',b+1)
  if a~= nil then
    StepInst = tonumber(s1)
  end
  
  -- Execute the instructions
  while(StepInst > 0) do
    pc = CPU_FetchEx()
    StepInst = StepInst - 1
  end
  
  -- Report the CPU state and next instruction
  CPU_ReportRegs()
  CPU_DisOpCode(pc)
  ListAddx = pc
end
-- end of CmdFilr.lua
