void setup(){
  pinMode(13, OUTPUT);
  Serial.begin(9600);
}

void loop(){
  digitalWrite(13, HIGH);
  delay(1000);
  Serial.println("on");
  digitalWrite(13, LOW);
  delay(1000);
  Serial.println("off");
}