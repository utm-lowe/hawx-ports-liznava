#include "console.h"
#include "port.h"
#include "disk.h"
#include "tests.h"
#include "string.h"
#include "riscv.h"

///////////////////////////////////////////////////////////////////////////////
// Unit Tests in this line should not be changed. You may study them to see
// how the tests are conducted, however.
///////////////////////////////////////////////////////////////////////////////

void 
test_uart() 
{
    char buf[100];
    int i=0;
    int p;

    // if we can print via flushing then we have initialized the uart
    printf("UART initialization test...");
    print_pass(1);
    uartflush();

    // test uartflush
    printf("UART flush test...");
    uartflush();
    print_pass(ports[PORT_CONSOLEOUT].count == 0);
    uartflush();

    // interrupt driven output
    printf("Interrupt driven output test...");
    p = ports[PORT_CONSOLEOUT].count != 0;
    intr_on();
    uartstart();
    while(ports[PORT_CONSOLEOUT].count);
    print_pass(p);

    // interrupt driven input test
    printf("Type the word \"PASSED\" and press enter: ");
    uartflush();
    i=0;
    do {
        while(port_read(PORT_CONSOLEIN, buf+i, 1) != 1);
        i++;
    } while (buf[i-1] != '\n');
    buf[i-1] = '\0';
    printf("Interrupt driven input test...");
    print_pass(strcmp(buf, "PASSED") == 0);
    intr_off();
}

//////////////////////////////////////////////////////////////////////
// Disk Tests
//////////////////////////////////////////////////////////////////////
struct disk_response 
{
    char mode;
    char status;
    int blockid;
};

static struct disk_response
await_disk_response(int dpm) 
{
    struct disk_response resp;
    char buf[10];

    // read the disk response string
    while(ports[dpm].count < 9) {
        virtio_disk_start();
    }
    port_read(dpm, buf, 9);
    buf[9] = '\0';
   
    // parse disk response
    resp.mode = buf[0];
    resp.status = buf[1];
    resp.blockid = atoi(buf+2);

    return resp;
}

void 
disk_test()
{
    char src[1025];
    char buf[1025];
    char c='A';
    int dpr;
    int dpw;
    int dpm;
    int p;
    struct disk_response resp;

    // generate a source string
    for(int i=0; i<1024; i+=8) {
        for(int j=0; j<8; j++) {
            src[i+j] = c;
        }
        c++;
        if(c > 'Z') c = 'A';
    }

    //null terminate the buffers
    src[1024] = '\0';
    buf[1024] = '\0';

    //acquire some ports
    dpr = port_acquire(-1, 0);
    dpw = port_acquire(-1, 0);
    dpm = port_acquire(-1, 0);


    // write the disk command messages
    //pprintf(PORT_DISKCMD, "R%7d%4d%4d", 1, dpr, dpm); 
    //pprintf(PORT_DISKCMD, "R%7d%4d%4d", 2048, dpw, dpm); 

    // turn on interrupts and run the disk tests
    // messages
    uartflush();
    intr_on();

    //populate the write port
    port_write(dpw, src, 1024);

    // send the write message to the disk
    //pprintf(PORT_DISKCMD, "R%7d%4d%4d", 2048, dpw, dpm); 

    // perform the write test
    printf("Writing to disk...");
    uartflush();
    pprintf(PORT_DISKCMD, "W%7d%4d%4d", 1, dpw, dpm); 
    resp = await_disk_response(dpm);
    print_pass(resp.status == 'S');

    // perform the read test
    printf("Reading from disk...");
    uartflush();
    pprintf(PORT_DISKCMD, "R%7d%4d%4d", 1, dpr, dpm); 
    resp = await_disk_response(dpm);
    port_read(dpr, buf, 1024);
    print_pass(resp.status == 'S' && strcmp(src, buf) == 0);

    // writing from an empty port
    printf("Empty port disk write...");
    uartflush();
    pprintf(PORT_DISKCMD, "W%7d%4d%4d", 2, dpw, dpm); 
    resp = await_disk_response(dpm);
    print_pass(resp.status == 'F');

    // writing from a partially empty port
    printf("Partial port disk write...");
    uartflush();
    port_write(dpw, "c", 1);
    pprintf(PORT_DISKCMD, "W%7d%4d%4d", 2, dpw, dpm); 
    resp = await_disk_response(dpm);
    print_pass(resp.status == 'F');

    // reading into a non-empty port
    printf("Non-empty port disk read...");
    uartflush();
    port_write(dpr, "c", 1);
    pprintf(PORT_DISKCMD, "R%7d%4d%4d", 2, dpr, dpm);
    resp = await_disk_response(dpm);
    print_pass(resp.status == 'F');

    // decriptor exhaustion test
    printf("Descriptor exhaustion test...");
    uartflush();
    for(int i=0; i<20; i++) {
        pprintf(PORT_DISKCMD, "R%7d%4d%4d", 2, dpr, dpm);
    }
    p = 1;
    for(int i=0; i<20; i++) {
        resp = await_disk_response(dpm);
        p=p & (resp.status == 'F');
    }
    port_read(dpr, buf, 1024);
    pprintf(PORT_DISKCMD, "R%7d%4d%4d", 2, dpr, dpm);
    resp = await_disk_response(dpm);
    print_pass(p & (resp.status == 'S'));


    intr_off();
    uartflush();


    port_close(dpr);
    port_close(dpw);
    port_close(dpm);
}



// Run unit tests on the port system (for debugging)
void 
port_test(void)
{
    int i;
    int passed;
    char buf[PORT_BUF_SIZE];

    // test to make sure the ports correctly initialize
    printf("port init test...");
    passed=1;
    for(i=0; passed && i < PORT_DISKCMD; i++) {
        if(ports[i].free || ports[i].owner != 0) passed=0;
    }
    for(i=PORT_DISKCMD+1; passed && i<NPORT; i++) {
        if(!ports[i].free) passed=0;
    }
    for(i=0; passed && i<NPORT; i++) {
        if(ports[i].count != 0 || ports[i].head != 0 || ports[i].tail !=0) 
            passed = 1;
    }
    print_pass(passed);


    // test writing
    printf("port write test...");
    passed=1;
    if(port_write(PORT_CONSOLEIN, "abc", 3) != 3) passed = 0;
    if(port_write(15, "abc", 3) != -1) passed = 0;
    for(i=0; passed && i<PORT_BUF_SIZE-3; i++) {
        if(port_write(PORT_CONSOLEIN, "a", 1) != 1) {
            passed=0;        
        }
    }
    if(port_write(PORT_CONSOLEIN, "a", 1) != 0) passed = 0;
    print_pass(passed);


    // test reading
    printf("port read test...");
    passed = 1;
    if(port_read(PORT_CONSOLEIN, buf, 3) != 3) passed = 0;
    if(buf[0] != 'a' || buf[1] != 'b' || buf[2] != 'c') passed=0;
    if(port_read(PORT_CONSOLEIN, buf, PORT_BUF_SIZE) != PORT_BUF_SIZE-3) passed = 0;
    if(port_read(15, "abc", 3) != -1) passed = 0;
    print_pass(passed);

    // test acquire/close
    printf("port acquire/close test...");
    passed=1;
    if(port_acquire(-1, 42) != 3) passed = 0;
    if(ports[3].owner!=42 || ports[3].free) passed = 0;
    if(port_acquire(PORT_CONSOLEOUT, 0) != -1) passed = 0;
    if(port_acquire(255, 15) != 255) passed = 0;
    if(ports[255].owner!=15 || ports[255].free) passed = 0;
    port_close(3);
    if(!ports[3].free || ports[3].owner != 0 || ports[3].head != 0 ||
       ports[3].count != 0) passed = 0;
    print_pass(passed);

    
}
