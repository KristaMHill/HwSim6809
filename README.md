HwSim6809 Hardware Simulator for MC6809 Systems
=========

Jan. 10, 2013 - This project is the experimental development of a 
hardware simulator for MC6809 based systems.  This project is also
courseware, being written for student use in a computer engineering
course that I teach.  This project is first inspired by the
perceived need for a 6809 system simulator capable of executing a
stub or "talker" program in a target system, and communicates
through a port, with a remote debugger user interface.

Operating systems such as Linux and Windows each have the means to
create a pair of serial ports that route to each each other.  Such
applications are referred to as a "null-modem emulator," or "serial
port redirector," or "relay for bidirectional data transfer."  For
Linux and Windows, socat and com0com each serve as examples.

It is hoped that such a simulator, when used with a debugger user
interface designed for use with actual hardware, will provide an
environment that is more realistic, more like "real" hardware,
than having a simulator integrated into the debugger.  It is not
a goal of this project to provide a bus level simulation, rather
for that, consider using a VHDL description and simator which may
be more appropriate.

I am coding with C and Lua, in an attempt to strike a balance between
flexibilty and performance.  Having the simulator core in C provides
performance where it counts most.  The C code is compiled to a library,
whereby the library functions are called from Lua. On Linux such a
library is called a ".so" or "shared object" and on Windows its called
a ".dll" or a "dynamic-link library."

For flexibilty I chose to also use a scripting language.  Among
interpreted languages, Lua is reported to be very fast.  Lua will be
used for the higher level coding and to provide a means to "attach"
modules that each model a peripheral device.

For portability, the C code will be as generic as possible, so that
the simulator can easily be built for various host systems.  I will
rely on Lua's cross-platform libraries to provide some independence
from the operating system in dealing with such things as a serial
communication port.  

In writing the code I'm finding that I like also being able to "run
the simulator," using a command line line environment.  Perhaps some
day a graphical user interface can be incorporated. Having even a
rudimentary simulator interface may be a benefit to my students.

HwSim6809 Hardare Simulator for MC6809 Systems
