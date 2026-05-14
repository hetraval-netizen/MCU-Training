#define RX_BFR_SIZE 64
volatile char rx_buffer[RX_BFR_SIZE];
volatile int head = 0;
volatile int tail = 0;

char cmd_buffer[RX_BFR_SIZE];
int cmd_index = 0;

