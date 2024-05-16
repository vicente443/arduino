#include <SPI.h>
#include <Ethernet.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

#define LED1 5
#define LED2 6

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 101);
IPAddress server_addr(127,0,0,1); // Cambia esta dirección IP por la del servidor MySQL
EthernetServer server(80);
EthernetClient client;
char user[] = "tu_usuario";
char password[] = "tu_contraseña";

MySQL_Connection conn((Client *)&client);

unsigned long led1OnTime = 0;
unsigned long led2OnTime = 0;

void setup() {
  Serial.begin(9600);
  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);

  Ethernet.begin(mac, ip);
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
  
  // Imprime la dirección IP del servidor MySQL configurada
  Serial.print("MySQL server IP address: ");
  Serial.println(server_addr);
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            sendHTTPResponse(client);
            break;
          } else if (currentLine.startsWith("GET ")) {
            handleLEDControl(currentLine);
            currentLine = "";
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}

void sendHTTPResponse(EthernetClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<h1>Control de LEDs</h1>");
  
  client.print("<p>Estado actual de LED 1: ");
  if (digitalRead(LED1) == HIGH) {
    client.print("Apagado");
    if (led1OnTime != 0) {
      client.print(" desde ");
      client.print(led1OnTime / 1000);
      client.print(" segundos");
    }
  } else {
    client.print("Encendido");
    if (led1OnTime != 0) {
      client.print(" desde ");
      client.print(led1OnTime / 1000);
      client.print(" segundos");
    }
  }
  client.println("</p>");

  client.print("<p>Estado actual de LED 2: ");
  if (digitalRead(LED2) == HIGH) {
    client.print("Apagado");
    if (led2OnTime != 0) {
      client.print(" desde ");
      client.print(led2OnTime / 1000);
      client.print(" segundos");
    }
  } else {
    client.print("Encendido");
    if (led2OnTime != 0) {
      client.print(" desde ");
      client.print(led2OnTime / 1000);
      client.print(" segundos");
    }
  }
  client.println("</p>");

  client.println("<p><a href=\"/?led1on\">Encender LED 1</a></p>");
  client.println("<p><a href=\"/?led1off\">Apagar LED 1</a></p>");
  client.println("<p><a href=\"/?led2on\">Encender LED 2</a></p>");
  client.println("<p><a href=\"/?led2off\">Apagar LED 2</a></p>");
  client.println("</html>");
}

void handleLEDControl(const String& line) {
  if (line.indexOf("/?led1off") != -1) {
    digitalWrite(LED1, HIGH);
    led1OnTime = 0;
    insertLedLog(1, "Apagado");
  }
  if (line.indexOf("/?led1on") != -1) {
    digitalWrite(LED1, LOW);
    led1OnTime = millis();
    insertLedLog(1, "Encendido");
  }
  if (line.indexOf("/?led2off") != -1) {
    digitalWrite(LED2, HIGH);
    led2OnTime = 0;
    insertLedLog(2, "Apagado");
  }
  if (line.indexOf("/?led2on") != -1) {
    digitalWrite(LED2, LOW);
    led2OnTime = millis();
    insertLedLog(2, "Encendido");
  }
}

void insertLedLog(int ledId, const char* state) {
  if (conn.connect(server_addr, 3306, user, password)) {
    Serial.println("Connected to MySQL server!");

    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    char query[128];
    sprintf(query, "INSERT INTO led_logs (led_id, state) VALUES (%d, '%s')", ledId, state);
    Serial.print("Query: ");
    Serial.println(query); // Imprime la consulta SQL
    cur_mem->execute(query);
    delete cur_mem;

    conn.close();
  } else {
    Serial.println("Connection failed.");
  }
}
