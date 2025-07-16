/* Using Pico/Pico2 with RFM95X LoRa transceiver module
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
 * GP10 (pin14)   PIR_PIN
 * ==> PIR HC-SR501 PIR motion detection module, needs 5V for reliable operation.
 * ==> Output is 3.3v
 *
 * For BME280 sensor, BME280 address is set to 0x77
  GPIO 0 (pin 1) I2C0 SDA/SDI - BME280 Sensor (STEMMA QT Blue)
  GPIO 1 (pin 2) I2C0 SCL/SCK - BME280 Sensor (STEMMA QT Yellow)
  (STEMMA QT 3.3V Red, GND Black)
  * 
 * Using modified RadioHead and Python libraries
 * June 2025.
*/

#include "RFM95fun.cpp" // function file
#include "RFM95.h" // header file
#include "bme280_i2c.hpp" // Setup i2c functionality
using namespace std;

/* ======================= MAIN PROGRAM ====================== */
int main() {
  unsigned char id; // register buffer
  stdio_init_all();  // set up to access USB via minicom
  sleep_ms(2000); // give minicom time to catch up
  printf("%c%c%c%c\n",0x1B,0x5B,0x32,0x4A); // clear screen from Pico end
  sleep_ms(500);
  pico_rfm_interface(); // setup Pico interface pins
  /* setup PIR pin for motion detection */
  gpio_init(PIR_PIN); // PIR module output signal, detect PIR status
  gpio_set_dir(PIR_PIN, GPIO_IN); // PIR_PIN is an input pin
  /* setup LED for blinking on message receipt */
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  manual_reset(); // Place all registers back to default values

  // This example will use i2c0, and GP0 and GP1 for BME280 sensor
  i2c_init(i2c0, 400 * 1000);
  gpio_set_function(0, GPIO_FUNC_I2C); // SDA
  gpio_set_function(1, GPIO_FUNC_I2C); // SCL
  gpio_pull_up(0);
  gpio_pull_up(1);

  //int32_t humidity, pressure, temperature;
  sleep_ms(1000);                         //  for 1 second
  //Setup BME280 Sensor 
  //Address of 0x77 is default for I2C line with SDO high
  BME280 Sensor(i2c0, 0x77);
    
  cout << "========================================" << endl;
  cout << "=     Prototype RFM95 Client Setup     =" << endl;
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
  // Calibrate PIR for 60 seconds
  calibratePIR();
  
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

  // Set transmitter power output, p.79
  bool useRFO = false; // 0 = RFO pin (limit +14 dBm), 1 = PA_BOOST (limit +20 dBm)
  int8_t power = 5;
  set_TX_power(power, useRFO);

  float pVar; // for BME280
  int32_t humidity, pressure, temperature; // for BME280
  
  // Check for motion and send message, otherwise send weather
  while(1) {
    Sensor.bme280_read(&humidity, &pressure, &temperature);
    //sleep_ms(10000); // pause for 10 seconds
    // Concatenate strings and variables together 
    if(temperature/100.0 >= 15.6) {pVar=3376.85; } else {pVar=3386.38; }
    string humVal = to_string(humidity / 1024.0); // convert double to string
    humVal = humVal.substr(0,4); // three digits
    double tpVal = (temperature/100.0)/.55555556+32; // find temperature fahrenheit
    string tempVal = to_string(tpVal); // convert double to string
    if(tpVal >=100) { tempVal = tempVal.substr(0,3); } else { tempVal = tempVal.substr(0,2); } // four digits
    string pressVal = to_string(pressure / pVar); // convert double to string
    pressVal = pressVal.substr(0,5); // five digits
    string message = "T: " + tempVal + " F\nH: " + humVal + " %\nP: " + pressVal + " inHg\n"; // assemble complete message
    
    build_packet(SERVER, CLIENT, identifier, flags, message);
    sleep_ms(5000); // 5 seconds
  }
    
  
/*  
  while (1) { // do nothing loop
    tight_loop_contents();
  } */
  return 0;
}
