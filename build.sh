#!/bin/bash                                                                                                           
set -e
# Auto-recover from a previously-interrupted build that left .ninja_deps                                                
# truncated. Cheap: ninja regenerates the file on the next build.                                                       
if cmake --build build --target hanabi_bot -j 2>&1 | tee /dev/stderr | grep -q 'premature end of file'; then            
    rm -f build/.ninja_deps build/.ninja_log                                                                              
    cmake --build build --target hanabi_bot -j                                                                            
fi