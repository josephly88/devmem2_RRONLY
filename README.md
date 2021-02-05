# devmem2_modify

# A modified version of devmem2 (https://github.com/hackndev/tools/blob/master/devmem2.c)
# Change to read only, print all bytes between start and end offset.

# Usage:
#  ./devmem2_modify {address} {start} [end]
#  address : memory address to act upon
#  start : decimal offset or 0x{heximal offset}
#  end : decimal offset or 0x{heximal offset} (Default: read 1 byte)
  
