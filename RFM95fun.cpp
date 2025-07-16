// RFM95fun.cpp
// Constructed from: RFM69HCW example

#include <RFM95.h>

using namespace std;

//volatile uint32_t counter = 0; //The counter variable for interrupt, TEST

static inline void cs_select() { // chip select line for SPI (NSS)
  asm volatile("nop \n nop \n nop");
  gpio_put(CS_PIN, 0); // Active low
  asm volatile("nop \n nop \n nop");
}
static inline void cs_deselect() { // chip select line for SPI (NSS)
  asm volatile("nop \n nop \n nop");
  gpio_put(CS_PIN, 1);
  asm volatile("nop \n nop \n nop");
}
/* Function to initialize Pico default SPI0 to RH_RF95 connections*/
static void pico_rfm_interface() {
  /* setup SPI interface */
  spi_init(spi0, 5000 * 1000); // 5 MHz ???
  gpio_set_function(RX_MISO, GPIO_FUNC_SPI); // SPI0 RX/MISO
  gpio_set_function(SCK, GPIO_FUNC_SPI); // SPI0 SCK
  gpio_set_function(TX_MOSI, GPIO_FUNC_SPI); // SPI0 TX/MOSI
  gpio_set_function(CS, GPIO_FUNC_SPI); // SPI0 CS
  // Chip select is active-low
  gpio_init(CS); // SPI0 CS (NSS)
  gpio_set_dir(CS, GPIO_OUT);
  /* Setup manual reset pin*/
  gpio_init(RESET_PIN);
  gpio_set_dir(RESET_PIN, GPIO_OUT);
  gpio_pull_up(RESET_PIN);
  /* Setup GPIO for RFM95 DIO0. See header file for actual Pico pin */
  gpio_init(DIO0);
  gpio_set_dir(DIO0, GPIO_IN);
  /* configure FIFO for RX and TX. */
  write_register(REG_0E_FIFO_TX_BASE_ADDR, TX_BASE);
  write_register(REG_0F_FIFO_RX_BASE_ADDR, RX_BASE);
}
/* Function to reset all register values to default 
   by resetting main transceiver board. NOTE: This is reversed
   from RFM69HCW reset procedure. */
static inline void manual_reset() { // RFM95 Datasheet, Ver 2.0, p. 111
  asm volatile("nop \n nop \n nop");
  gpio_put(RESET_PIN, 0); // set low
  sleep_us(100); // keep low for 100 uS
  gpio_put(RESET_PIN, 1); // set high
  sleep_ms(10); // chip ready after 5 mS
}

/* Function to read registers */
static void read_register(uint8_t reg, uint8_t *buf, uint16_t len) {
  reg &= READ_BIT; // set wnr, reg bit8 to 0 for read
  //reg.set(8,0); // wnr bit is 0 for read
  cs_select();
  spi_write_blocking(spi0, &reg, 1);
  //sleep_ms(10);
  spi_read_blocking(spi0, 0, buf, len);
  cs_deselect();
  sleep_ms(10);
}

/* Function to write a register */
static void write_register(uint8_t reg, uint8_t data) {
  uint8_t buf[2];
  buf[0] = reg |= WRITE_BIT; // wnr bit8 to 1 for write access
  buf[1] = data;
  cs_select(); // active low
  spi_write_blocking(spi0, buf, 2);
  cs_deselect(); // set high
  sleep_ms(10);
}
/* New function to read particular registers */
static void showRegisters() {
  unsigned char id;
  cout << "Reading particular registers..." << endl;
  uint8_t regSet[] = { 0x01, 0x06, 0x07, 0x08, 0x0d, 0x0e, 0x0f, 0x10, 0x1d, 0x1e, 0x26, 0x09 }; // desired register addresses
  uint8_t i;
    for (i = 0; i < sizeof(regSet); i++) { // length of array above
      read_register(regSet[i], &id,2); // read register and value, then print as below.
      if (i == 0) cout << "--- OpMode:" << endl;
      if (i == 1) cout << "--- Frequency (frf):" << endl;
      if (i == 4) cout << "--- FIFO AddrPtr, TX base, RXbase, Current:" << endl;
      if (i == 8) cout << "--- Modem status:" << endl;
      if (i == 11) cout << "--- PA config:" << endl;
      cout << "Register: 0x" << hex << static_cast<int>(regSet[i]) << ", Value: " << bitset<8>(id) << ", 0x" << hex << static_cast<int>(id) << endl;
    }
}
/* Function to read frequency */
static void frequency() {
  unsigned char id;
  int msb,mid,lsb;
  read_register(static_cast<int>(REG_06_FRF_MSB), &id, 2);
  msb = id;
  read_register(static_cast<int>(REG_07_FRF_MID), &id, 2);
  mid = id;
  read_register(static_cast<int>(REG_08_FRF_LSB), &id, 2);
  lsb = id;
  uint32_t final = (FSTEP * ((msb << 16) | (mid << 8) | lsb)) /1000000;
  cout << "\nCarrier frequency: " << dec << static_cast<int>(final) << " MHz.\n" << endl;
}
/* Function to set frequency */
static uint8_t set_frequency(double FREQ) { // set operating frequency
  int frf = (FREQ * 1000000) / FSTEP;
  write_register(REG_06_FRF_MSB, (frf >> 16) & 0xff);
  write_register(REG_07_FRF_MID, (frf >> 8) & 0xff);
  write_register(REG_08_FRF_LSB, frf & 0xff);
  return 0;
}
/* Set modem parameters */
static uint8_t set_modem(uint8_t Modem1, uint8_t Modem2, uint8_t Modem3) {
  write_register(REG_1D_MODEM_CONFIG1, Modem1);
  write_register(REG_1E_MODEM_CONFIG2, Modem2);
  write_register(REG_26_MODEM_CONFIG3, Modem3);
  return 0;
}

// convert hex characters to ASCII characters
static vector<uint8_t> hex2ascii(vector<uint8_t> payload) {
  for(auto i:payload) { // automatically find length of 'hx'
        cout << (char) static_cast<int>(i); // hex to int to char
    } 
    cout << endl;
    return payload;
}

/* Function to send message */
static uint8_t send_message(uint8_t destination, uint8_t sender, uint8_t identifier, uint8_t flags, uint8_t* data, uint8_t len) {
  write_register(REG_01_OP_MODE, STBY_MODE); // LoRa mode changed only in Sleep mode.
  write_register(REG_0D_FIFO_ADDR_PTR, REG_0E_FIFO_TX_BASE_ADDR); // FIFO ptr to TX base
  // Header data
  write_register(REG_00_FIFO, destination);
  write_register(REG_00_FIFO, sender);
  write_register(REG_00_FIFO, identifier);
  write_register(REG_00_FIFO, flags);
    
  for(int i = 0; i <= len; i++) { // fill FIFO register
      write_register(REG_00_FIFO, data[i]);
  }
  write_register(REG_22_PAYLOAD_LENGTH, len + 4); // fill payload length
  // set TX mode
  write_register(REG_40_DIO_MAPPING1, TX_DONE); // set DIO0 for TxDone interrupt
  write_register(REG_01_OP_MODE, TX_MODE); // transmit packet
  unsigned char id; // for register buffer, TEST
  read_register(REG_01_OP_MODE, &id, 2); // transmit mode, TEST
  if (id == 0x83) {
     cout << "\nTransmit Mode." << endl; // test for transmit mode, TEST
   } else { cout << "BAD Mode." << endl; } // TEST
   while (!DIO0) {
    tight_loop_contents();
  }
   cout << "Packet sent." << endl;
  // mode returns to standby
  
  // read FIFO, TEST
  write_register(REG_0D_FIFO_ADDR_PTR, REG_0E_FIFO_TX_BASE_ADDR); // FIFO ptr to TX base

  for(int i = 0; i <= len+4; i++) {
    read_register(REG_00_FIFO, &id, 1);
    cout << static_cast<int>(id) << " "; // show in hex
    //cout << (char) static_cast<int>(id); // show in ascii
  }
  cout << endl;
  read_register(REG_01_OP_MODE, &id, 2); // TEST
  if (id == 0x81) {
    cout << "Standby Mode." << endl; // test for receive mode, TEST
  } else { cout << "BAD Mode." << endl; } // TEST
  // end of read FIFO, TEST 

  return 0;
}
static int build_packet(uint8_t SERVER,uint8_t CLIENT,uint8_t identifier,uint8_t flags, std::string message) {
  write_register(REG_01_OP_MODE, STBY_MODE); // LoRa FIFO can only be filled in stand-by mode

  if (checkPIR()) { // get PIR pin status
    message = "\n===> MOTION DETECTED! <===\n";
    blinkLED();
  }
  write_register(REG_0D_FIFO_ADDR_PTR, TX_BASE); // set message start point
  auto mLen = message.size()+4;
  // add length + 4 header bytes and message to payload vector, then put in Fifo
  vector<uint8_t> payload; 
  write_register(REG_00_FIFO, SERVER);
  write_register(REG_00_FIFO, CLIENT);
  write_register(REG_00_FIFO, identifier);
  write_register(REG_00_FIFO, flags);
  for(const int& i : message) { write_register(REG_00_FIFO, i); }
  uint8_t val = 0;
  // set TX mode
  write_register(REG_22_PAYLOAD_LENGTH, mLen); // set payload length
  write_register(REG_40_DIO_MAPPING1, DIO0_TXdone); // set DIO0 for TxDone interrupt
  write_register(REG_12_IRQ_FLAGS, 0xff); // clear IRQ flags
  write_register(REG_01_OP_MODE, TX_MODE); // transmit packet
  while(!val) { // wait for TX_DONE flag
    val = packet_sent();
  }
  // After TX_DONE flag, radio returns to STBY_MODE
  cout << "Packet sent." << endl;

  return 0;
}

static int packet_sent() { // check for TX_DONE flag, 0x08
  unsigned char id;
  read_register(REG_12_IRQ_FLAGS, &id, 2);
  uint8_t val = (id & TX_DONE) >> 3; // move bit 4 to bit 1
  //if (val) cout << "TX_DONE val: " << static_cast<int>(val) << endl; // TEST
  return val;
}

static int payload_ready() { // check for RX_DONE flag, 0x40
  unsigned char id;
  read_register(REG_12_IRQ_FLAGS, &id, 2);
  uint8_t val = (id & RX_DONE) >> 6; // move bit 7 to bit 1
  //if (val) cout << "RX_DONE val: " << static_cast<int>(val) << endl; // TEST
  
  return val;
}

/* Function to receive message */
static int receive(uint8_t SERVER, uint8_t CLIENT) {
  uint8_t val = 0;
  write_register(REG_01_OP_MODE, RX_CONT_MODE); // LoRa mode changed only in Sleep mode. Set receive continuous mode
  write_register(REG_40_DIO_MAPPING1, DIO0_RXdone); // RXDone flag
  //write_register(REG_0D_FIFO_ADDR_PTR, RX_BASE); // set start point back
  cout << "Receive mode. Waiting for message..." << endl; // TEST
  while(!val) { // wait for RX_DONE flag
    val = payload_ready();
  }
  write_register(REG_12_IRQ_FLAGS, 0xff); // clear IRQ flags
  vector<uint8_t> payload = check_rxbuf();
  uint8_t id; //,rssi,pktRSSI;

  unsigned char id1 = payload.size();
    if (id1 >= 4) { // check for good header
    uint8_t h_to = payload[0];
    uint8_t h_from = payload[1];
    uint8_t h_id = payload[2];
    uint8_t h_flags = payload[3];
    if (h_to == SERVER) { // Is this message for this server? If so, process.
      cout << "\nHeader: " << hex << static_cast<int>(h_to) << " " << static_cast<int>(h_from) << " " << static_cast<int>(h_id) << " " << static_cast<int>(h_flags) << endl; // TEST
      for(int i=4;i<id1-1;i++) { cout << (char) static_cast<int>(payload[i]); } // show ASCII characters
      cout << endl;
      
      // get SNR (twos complement), and RSSI values
      read_register(REG_19_PKT_SNR_VALUE, &id, 2); // get SNR value, 2s complement value ???
      int8_t snr = (id/4.0);
      read_register(REG_1B_RSSI_VALUE, &id, 2); // get RSSI value, p. 83
      int8_t rssi = static_cast<int8_t>(id);
      read_register(REG_1A_PKT_RSSI_VALUE, &id, 2); // get RSSI value, p. 83
      int8_t pktRSSI = static_cast<int8_t>(id);
      
      if( snr < 0) { 
        rssi = (-157) + rssi + snr; // use snr in calculation
        printf("RSSI: %d dBm\n", rssi); // cout WILL NOT display correctly
      } else { rssi = (-157) + (16/15) * rssi;
        printf("RSSI: %d dBm\tSNR: %d dB\n", rssi,snr); // cout WILL NOT display correctly
      }
    }
  }
 
  write_register(REG_01_OP_MODE, STBY_MODE); // return to standby/idle
  return 0;
}

/* Function to retrieve message from FIFO buffer */
static std::vector<uint8_t> check_rxbuf() {
  unsigned char id, id1;
  read_register(REG_13_RX_NB_BYTES, &id, 2); // latest packet length
  id1 = id;
  read_register(REG_10_FIFO_RX_CURRENT_ADDR, &id, 2); // get current address
  write_register(REG_0D_FIFO_ADDR_PTR, id); // set addr pointer to current address
  vector<uint8_t> payload;
  for(int i=0;i<=id1;i++) { // read Fifo register
    read_register(REG_00_FIFO, &id, 1);
    payload.push_back(id); // assemble payload from FIFO
  }

  return payload;
}
// Set transmitter power level, p. 79 (Has not been tested)
static void set_TX_power(int8_t power, bool useRFO) {
  if (useRFO) {
    if (power > 14) power = 14;
    if (power < -1) power = -1;
    write_register(REG_09_PA_CONFIG, MAX_POWER | (power + 1)); // p.104
  } else {
    if (power > 23) power = 23;
    if (power < 5) power = 5;
    if (power > 20) {
      write_register(REG_4D_PA_DAC, PA_DAC_ENABLE); // p. 100
      power -= 3;
    } else {
      write_register(REG_4D_PA_DAC, PA_DAC_DISABLE); // p.100
    }
    write_register(REG_09_PA_CONFIG, PA_SELECT | (power - 5)); // p. 104
  }
}
// Blink LED for Server "msg rcvd," or Client "PIR signal."
static void blinkLED() {
  gpio_put(LED_PIN, 1); // on
  sleep_ms(250);
  gpio_put(LED_PIN, 0); // off
  sleep_ms(250);
}

// Delay function used to calibrate PIR sensor
static void calibratePIR() {
  cout << "Calibrating PIR." << endl;
  for (uint counter = 0; counter < 60; counter++){
    blinkLED();
  }    
  cout << "\nCalibration completed" << endl;
}
// function to check status of PIR pin for motion
static int checkPIR() {
  if (gpio_get(PIR_PIN)) { // get pin status
    cout << "===> MOTION DETECTED! <===" << endl;
    return 1; // true
  } else { return 0; // false
  }
}
