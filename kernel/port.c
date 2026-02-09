
//    This means that whenever data arrives into the buffer, the buffer
//    actually remembers the order in which data arrived. This will retrieve
//    you with the oldest      data first. So the first data that enters is the
//    first data that leaves the buffer(FIFO).
//
//       - HEAD (H) and TAIL(T) are the locations in the buffer, where data is
//         being deposited and retrieved respectively
//
//       - A circular buffer first starts out empty and has a set length.
//
//    Let's consider below a five-element buffer as an array. In the array, you
//    have variables which keep track of a head and a tail, and they use the
//    indices of the   array as the variable they store.
//
// Step 1:
//       - First, both HEAD and TAIL will be on the same cell.
//
//        H T
//     |------------ |------------|------------|-----------|-----------|
//     |             |            |            |           |           |
//     |-------------|------------|------------|-----------|-----------|
//      HEAD (H) = 0
//      TAIL (T) = 0
//
//    When we insert data into buffer, first thing the function has to do is
//    check if there is any room available in the buffer.
//
//       - One way to do this is, you can take the difference between head and
//         tail, a tail and head depending on which one's larger and compare
//         that to how many spaces  are in the buffer.
//
//       - Other way, (optimal) is to use another variable called COUNT(C),
//         when the buffer is empty, we say COUNT equals to 0 (i.e, COUNT=0).
//         Now the function that we    called, comes in and checks if the COUNT
//         is less than five(which is the size of the buffer),goes ahead and
//         accepts data. Let's continue with this way.
// Step 2:
//    Let's say, we bring in number 5 (number 5 is written to the array) and
//    then several things get updated.
//
//       - H (head)has to get moved up one and in reality the way it actually
//         happens is the index of where head is is incremented to one 
//         (i.e, HEAD = 1)
//
//       - Also, as a peice of data is stored in our array, the COUNT is also
//         incremented to one (i.e, COUNT=1)
//
//        T            H
//    |------------ |------------|------------|-----------|-----------|
//    |     5       |            |            |           |           |
//    |-------------|------------|------------|-----------|-----------|
//      HEAD (H) = 1
//      TAIL (T) = 0
//      COUNT (C) = 1
//
// Step 3:
//    Now that we have a basic understanding of how data comes in, let's speed
//    the process up.  
//
//    Let's say another 3 bytes of data comes in. Ex 7,2,12.
//
//       - So everytime, when each one of these data comes in, the head gets
//         incremented by one and count gets incremented by one.
//
//       - Since three more pieces of data came in, both head and count go by
//         three.
//
//          T                                                   H
//    |------------ |------------|------------|-----------|-----------|
//    |     5       |      7     |     2      |     12    |           |
//    |-------------|------------|------------|-----------|-----------|
//       HEAD (H) = 4
//       TAIL (T) = 0
//       COUNT (C) = 4
//
// Step 4:
//    Now things become little tricky. Because we're usig arrays, arrays always
//    start at index 0 which makes things just a little more to work with.
//
//    Let's say our next byte of data comes in Ex 3
//
//       - As, size of our array is five, we have to wrap it back around and
//         now head gets back to zero. As arrays incremented from four to five,
//         COUNT will be five.
//
//         H T
//    |------------ |------------|------------|-----------|-----------|
//    |     5       |      7     |     2      |     12    |     3     |
//    |-------------|------------|------------|-----------|-----------|
//          ^                                                    |
//          |----------------------------------------------------|
//       HEAD (H) = 0
//       TAIL (T) = 0
//       COUNT (C) = 5
//
// Step 5:
//    Next time when data writes into the buffer, the COUNT lets the function
//    know that the array(buffer) is full. Here, we can decide how the buffer
//    should behave.(Override or stop writing)
//
//       - One method when a data is read out of the buffer is, it first checks
//         whether the COUNT is greater than zero, if yes, it lets you return
//         that data.
//
//       - So we store the 5, then we move the TAIL from zero to one and we
//         decrement the COUNT.
//
//          H              T
//    |------------ |------------|------------|-----------|-----------|
//    |     5       |      7     |     2      |     12    |     3     |
//    |-------------|------------|------------|-----------|-----------|
//          ^                                                    |
//          |----------------------------------------------------|
//    HEAD (H) = 0
//    TAIL (T) = 1
//    COUNT (C) = 5
//
//  Note: Once the tail gets all the way to the end, we have to follow the same
//    kind of modulo increment. i.e, if the tail gets to 5 in our case, the
//    function knows to wrap the tail back around. Once the tail and count are
//    incremented, we take the value 5 that was stored previously and go ahead
//    and return at which exit out of the function.
//
//    Also, there is one more function apart from the functions that put data
//    into the buffer and data out of the buffer, that is 'peak'. It is very
//    similar to the get data out of the buffer but the difference being is
//    that peek retrieves the data out of the buffer without effectively
//    erasing it. In other words, without incrementing  the tail  and
//    decrementing the count.


#include "types.h"
#include "riscv.h"
#include "console.h"
#include "port.h"

// The global collection of ports
struct port ports[NPORT];


// Initialize the ports
void 
port_init(void)
{




  for(int i = 0; i < NPORT; i++){
    ports[i].count = 0;
     ports[i].tail = 0;
    ports[i].free = 1;

    ports[i].head = 0;
    
    ports[i].owner = -1;
    ports[i].type = PORT_TYPE_FREE;



    if(i == 0){
        ports[i].owner = 0;
        ports[i].type = PORT_TYPE_KERNEL;


        ports[i].free = 0;
    }


    else if(i == 1){
       

        ports[i].owner = 0;
        ports[i].type = PORT_TYPE_KERNEL;

        ports[i].free = 0;
    }
    else if(i == 2){


        ports[i].owner = 0;

        ports[i].type = PORT_TYPE_KERNEL;

        ports[i].free = 0;
    }
}


}

void 
port_close(int port)
{

    if(ports[port].free == 1) return;
    

    if(ports[port].type == PORT_TYPE_KERNEL) return;


    ports[port].tail = 0;
    ports[port].count = 0;
    ports[port].free = 1; 
    ports[port].head = 0;

    ports[port].owner = 0;
    ports[port].type = PORT_TYPE_FREE;



}



int port_acquire(int port, procid_t proc_id)
{

    if(port == -1){
        for(int i = PORT_DISKCMD + 1; i < NPORT; i++){

            if(ports[i].free == 1){
                ports[i].free  = 0;



                ports[i].owner = proc_id;

                return i;

            }
        }
        return -1;

    }

    if(port >= NPORT) 
        return -1;


    if(ports[port].type == PORT_TYPE_KERNEL) 
        return -1;

    if(ports[port].free == 0) return -1;

    ports[port].free  = 0;

    ports[port].owner = proc_id;

    return port;
}

// Write up to n characters from buf to a port.  Return the number of bytes written.
int 
port_write(int port, char *buf, int n)
{
    




    if(ports[port].free == 1 && ports[port].type != PORT_TYPE_KERNEL){
        return -1;
    }


    int k = 0;
    while(k < n && ports[port].count < PORT_BUF_SIZE){
        ports[port].buffer[ports[port].head] = buf[num];

        ports[port].head = (ports[port].head+1) % PORT_BUF_SIZE;

        k++;
        ports[port].count++;

    }

    return k; 

}
int 
port_read(int port, char *buf, int n)
{
    
    if(ports[port].free == 1 && ports[port].type != PORT_TYPE_KERNEL){
        return -1;
    }

    int k = 0;
    while(k < n && ports[port].count > 0){


        buf[k] = ports[port].buffer[ports[port].tail];

        ports[port].tail = (ports[port].tail+1) % PORT_BUF_SIZE;

        ports[port].count--;
        k++;
    }

    return k; 

}