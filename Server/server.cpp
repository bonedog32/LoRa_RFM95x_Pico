/* Using Pico/Pico2 with RFM95C LoRa transceiver module
 * Pin connections to XCVR module as in Python working example:
 * Pico           LoRa
 * ------------   ----------
 * 3V3            VIN
 * GND            GND
 * GP28 (pin34)   G0/DIO0
 * GP6 (pin9)     SCK
 * GP4 (pin6)     MISO/RX
 * GP7 (pin10)    MOSI/TX
 * GP5 (pin7)     CS
 * GP27 (pin32)   RST
 * GP25 internal  LED_PIN
 *
 * Using modified RadioHead and Python libraries
 * June 2025.
*/

#include "RFM95fun.cpp" // function file
#include "RFM95.h" // header file
using namespace std;

/* ======================= MAIN PROGRAM ====================== */
int main() {
  unsigned char id; // register buffer
  stdio_init_all();  // set up to access USB via minicom
  sleep_ms(2000); // give minicom time to catch up
  printf("%c%c%c%c\n",0x1B,0x5B,0x32,0x4A); // clear screen from Pico end
  sleep_ms(500);
  pico_rfm_interface(); // setup Pico interface pins
  manual_reset(); // Place all registers back to default values
    
  
  cout << "========================================" << endl;
  cout << "=     Prototype RFM95 Server Setup     =" << endl;
  cout << "========================================" << endl;
    
  // get chip version
  read_register(REG_42_VERSION, &id, 2);
  cout << "Chip version: 0x" << hex << static_cast<int>(id) << ", Value: " << bitset<8>(id) << endl;
    
  // Set modes Sleep, Lora. LoRa mode can only be set in sleep mode.
  write_register(REG_01_OP_MODE, LONG_RANGE_MODE | SLEEP_MODE); // Lora mode, sleep mode
  read_register(REG_01_OP_MODE, &id, 2); // TEST
  cout << "\nOpMode: 0x" << hex << static_cast<int>(id) << ", Value: " << bitset<8>(id) << endl;
  if (id == 0x80) {
    cout << "--> LoRa Mode, Sleep Mode." << endl;
  } else { cout << "Mode incorrect!" << endl;
  }
  double FREQ = 915.0;
  set_frequency(FREQ); // set frequency to FREQ
  frequency(); // show frequency
  
  // Define transceiver parameters.
  /* MODEM1: Sig. BW = 125 kHz, Error Coding Rate = 4/5, default Explicit header mode */
  uint8_t Modem1 = BW_125KHZ | CODING_RATE_4_5; // 0x72; 
  /* MODEM2: Spreading Factor = 128 chips/symbol, CRC enabled */
  uint8_t Modem2 = SPREADING_FACTOR_128CPS | PAYLOAD_CRC_ON; // 0x74;
  /* MODEM3: LNA gain set by internal AGC */
  uint8_t Modem3 = AGC_AUTO_ON; // 0x04;
  set_modem(Modem1, Modem2, Modem3); // Set transceive parameters
  
  //showRegisters(); // show registers for frequency, modems, FIFO, TEST
  
  // Setup addresses
  uint8_t CLIENT = 0x01; // this node, sender
  uint8_t SERVER = 0x02; // recipient of packet
  uint8_t identifier = 0x00; // sequence count for reliable datagram mode
  uint8_t flags = 0x00; // 

  //gpio_set_irq_enabled_with_callback(DIO0, RXdone, true, &test_callback); // TEST
  
  while (1) {
	  // set receive function
	  receive(SERVER, CLIENT); // goto receive continuous mode and wait for RXdone interrupt
	  blinkLED();
  }
  
/*  
  while (1) {
    tight_loop_contents();
  } */
  return 0;
}
