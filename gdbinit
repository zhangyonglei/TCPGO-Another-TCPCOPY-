python
import sys
sys.path.insert(0, '/mnt/hgfs/e_drive/gdb-pretty')
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers (None)
end
