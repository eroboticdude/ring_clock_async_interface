void timeDisplay5(byte h, byte m, byte s) { //all lights white, but dimmer 
  //minute dial
  if (h > 11) {
    h = h - 12; //(24 hr clock to 12)
  }
  //  hour dials
  
 fadeToBlackBy (leds, NUM_LEDS, 100);
  
  //for (int i = 0; i < NUM_LEDS; i += 5 ) {  //make every hour position brighter
    //for (int i = 0; i < NUM_LEDS; i += 15) {
    //leds[i] = CRGB(50, 50, 50);
 // }
  //time
  //hour
  
  if (m < 12) {
    leds[(h * 5) ] = HourColor; //h * 5 scales hour to 60 for clock, and the minutes dictate the hour hand inbetweens
  }
  if (m >= 12 && m < 24) {
    leds[h * 5 + 1] = HourColor;
  }
  if (m >= 24 && m < 36) {
    leds[h * 5 + 2] = HourColor;
  }
  if (m >= 36 && m < 48) {
    leds[h * 5 + 3] = HourColor;
  }
  if (m >= 48 && m < 60) {
    leds[h * 5 + 4] = HourColor;
  }
  //minute
  //leds[m / (60/NUM_LEDS)] = MinuteColor;
  leds[m] = MinuteColor;
  //scond
  //leds[s / (60/NUM_LEDS)] = SecondColor;
  leds[s] = SecondColor;
}
