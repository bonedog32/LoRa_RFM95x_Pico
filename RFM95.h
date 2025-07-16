// Header file for RFM95 module

 // Specify libraries and programs
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include <cstdint> // integer types definition,C++11
#include <iostream> // for user input/output
#include <cstring> // for text manipulation
#include "RFM95reg.h" // register definitions
#include "hardware/gpio.h"
#include "hardware/spi.h"

#include <vector> // manipulate vectors
#include <bitset> // show zeros and ones of variable
#include <iomanip> // show integer in hex
#include <string> // for getSensor message

// User defined definitions
#define RESET_PIN 27 // RST pin, GP27 (pin32) for RFM95 module
#define DIO0 28 // DIO0 for TXdone, RXdone, CADdone
#define WRITE_BIT 0x80 // wnr bit, 1 = write access
#define READ_BIT 0x7f // wnr bit, 0 = read access
#define CS_PIN 5 // CS pin (pin7)
#define RX_MISO 4 // RX/MISO
#define TX_MOSI 7 // TX/MOSI
#define SCK 6 // SCK
#define CS 5 // Chip Select
#define PIR_PIN 10 // GP 10 (pin 14)
#define LED_PIN 25 // built-in LED

#define DIO0_TXdone 0x40 // TX done interrupt, top 2 bits
#define DIO0_RXdone 0x00 // RX done interrupt, top 2 bits
#define DIO0_CADdone 0x80 // Channel Activity Detection interrupt, top 2 bits

#define FLAGS_ACK 0x80
#define BROADCAST_ADDRESS 255
#define RX_BASE 0x00 // Start of FIFO RX buffer, default. Changing causes issues.
#define TX_BASE 0x80 // Start of FIFO TX buffer, default. Changing causes issues.

// defined modes
#define LONG_RANGE_MODE 0x80 // LoRa Mode
#define SLEEP_MODE 0x00 // Sleep Mode
#define STBY_MODE 0x01 // Standby Mode
#define TX_MODE 0x03 // Transmit Mode
#define RX_CONT_MODE 0x05 // Receive Continuous Mode
#define RX_SINGLE_MODE 0x06 // Receive Single Mode

// This is the maximum number of interrupts the driver can support
// Most Arduinos can handle 2, Megas can handle more
#define NUM_INTERRUPTS 3

// Max number of octets the LORA Rx/Tx FIFO can hold
#define FIFO_SIZE 255

// This is the maximum number of bytes that can be carried by the LORA.
// We use some for headers, keeping fewer for RadioHead messages
#define MAX_PAYLOAD_LEN FIFO_SIZE

// The length of the headers we add.
// The headers are inside the LORA's payload
#define HEADER_LEN 4

// This is the maximum message length that can be supported by this driver. 
// Can be pre-defined to a smaller size (to save SRAM) prior to including this header
// Here we allow for 1 byte message length, 4 bytes headers, user data and 2 bytes of FCS
#ifndef MAX_MESSAGE_LEN
 #define MAX_MESSAGE_LEN (MAX_PAYLOAD_LEN - HEADER_LEN)
#endif

// The crystal oscillator frequency of the module is 32 MHz
#define FXOSC 32000000.0

// The Frequency Synthesizer step = FXOSC / 2^19,
// so Carrier Frequency = Fstep x Frf(23:0)
#define FSTEP  (FXOSC / 524288)

// Function prototypes
static void pico_rfm_interface(); // setup Pico board inputs/outputs/functions
static inline void manual_reset(); // RFM69HCW, Ver 1.1, p. 76
static void showRegisters(); // read most registers
static inline void cs_select(); // chip select active low
static inline void cs_deselect(); // chip de-select high
static void write_register(uint8_t reg, uint8_t data); // Write data to register
static void read_register(uint8_t reg, uint8_t *buf, uint16_t len); // read selected registers
static uint8_t set_frequency(double FREQ); // set operating frequency
static uint8_t set_modem(uint8_t Modem1, uint8_t Modem2, uint8_t Modem3); // setup modem parameters
static uint8_t send_message(uint8_t destination, uint8_t sender, uint8_t identifier, uint8_t flags, uint8_t* data, uint8_t len);
static int build_packet(uint8_t destination,uint8_t sender,uint8_t identifier,uint8_t flags, std::string message); // send message
static int receive(uint8_t SERVER, uint8_t CLIENT); // set receive mode and wait for payload
static int payload_ready(); // wait for RXdone interrupt
static int packet_sent(); // wait for TXdone interrupt
static std::vector<uint8_t> check_rxbuf(); // get packet
static void set_TX_power(int8_t power, bool useRFO);
static void blinkLED(); // blink built-in LED
static void calibratePIR(); // delay function for PIR calibration
static int checkPIR(); // check PIR for movement, (PIR_PIN high)
//extern string getSensor(); // get BME280 sensor data
