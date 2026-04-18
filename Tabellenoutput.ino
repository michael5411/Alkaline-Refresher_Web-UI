// Hilfsfunktion: gibt eine Fließkommazahl im Format X.YY aus
void printFixedXY(float v) {
  // positive Werte
  int whole = (int)v;
  int frac = (int)round((v - whole) * 100.0);
  if (frac == 100) { // Rundungsrandfall, z.B. 1.999 -> 2.00
    whole += 1;
    frac = 0;
  }
  // führende Null bei frac, falls nötig
  if (frac < 10) {
    Serial.print(whole);
    Serial.print('.');
    Serial.print('0');
    Serial.print(frac);
  } else {
    Serial.print(whole);
    Serial.print('.');
    Serial.print(frac);
  }
}

// Baut und druckt die Tabelle
void printTable() {
  Serial.println();
  Serial.println("       --- Batteriespannungen --- ");
  Serial.println();

  // Kopfzeile mit Spaltennummern (breite 9 pro Spalte, anpassbar)
  const int colWidth = 9;
  Serial.print(" "); // Ausrichtung
  for (int i = 0; i < 4; i++) {
    int pad = colWidth - 1; // eine Position für Abstand
    String s = String(i + 1);
    int leftSpaces = pad - s.length();
    for (int j = 0; j < leftSpaces; j++) Serial.print(' ');
    Serial.print(s);
  }
  Serial.println();
  Serial.println();

  // Wertezeile: jede Zahl rechtsbündig in der Spaltenbreite
  for (int i = 0; i < 4; i++) {
    // formatiere Wert in temporären String
    String tmp;
    {
      int whole = (int)battU[i];
      int frac = (int)round((battU[i] - whole) * 100.0);
      if (frac == 100) { whole += 1; frac = 0; }
      if (frac < 10) tmp = String(whole) + ".0" + String(frac);
      else tmp = String(whole) + "." + String(frac);
    }
    // rechtsbündig in colWidth
    int pad = colWidth - tmp.length();
    for (int j = 0; j < pad; j++) Serial.print(' ');
    Serial.print(tmp);
  }
  Serial.println();
}