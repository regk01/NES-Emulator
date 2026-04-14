import logging
import sys
import ctypes

# Setup standard Python logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s [%(levelname)s] CPU: %(message)s',
    handlers=[logging.StreamHandler(sys.stdout)]
)
logger = logging.getLogger("NES")

# Define the callback type for C
# void callback(const char* message, int level)
LOG_CALLBACK_TYPE = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_int)

def python_log_callback(message, level):
    if isinstance(message, bytes):
        msg = message.decode('utf-8')
    else:
        msg = str(message)
    if level == 0: logger.debug(msg)
    elif level == 1: logger.info(msg)
    elif level == 2: logger.warning(msg)
    else: logger.error(msg)

# Keep a reference to prevent garbage collection
c_log_func = LOG_CALLBACK_TYPE(python_log_callback)