# Branches du projet

## Module 1 — Bases Arduino / ESP32

| Branche             | Description                                                                   |
| ------------------- | ----------------------------------------------------------------------------- | ------------ |
| `module-1/step-1`   | LED rouge + bouton : `digitalWrite`, `digitalRead`, boucle simple.            |
| `module-1/step-2`   | LED jaune + bouton : même concept, second GPIO.                               | Reste allumé |
| `module-1/step-3`   | LED verte sans bouton : clignotement automatique, introduction du `delay`.    |
| `module-1/step-4`   | LED bleue + potentiomètre : lecture analogique ADC et PWM avec `analogWrite`. |
| `module-1/step-5`   | Plusieurs LEDs + boutons combinés : gestion de plusieurs GPIOs en parallèle.  |
| `module-1/step-6`   | Connexion WiFi : rejoindre un réseau et afficher l'IP dans le Serial.         |
| `module-1/step-7`   | WiFi + LCD : afficher l'IP et l'état de connexion sur l'écran I2C.            |
| `module-1/step-8`   | MQTT publish : envoyer des données au broker depuis l'ESP32.                  |
| `module-1/step-9`   | MQTT subscribe : recevoir des commandes depuis le broker.                     |
| `module-1/step-10`  | MQTT bidirectionnel : piloter plusieurs LEDs via topics MQTT.                 |
| `module-1/projet-1` | Projet : station de mesure avec DHT11 et LCD, données publiées en MQTT.       |
| `module-1/projet-2` | Projet : machine à états pilotée par MQTT avec alarme et pause.               |

## Module 2 — BMS

| Branche           | Description                                                                  |
| ----------------- | ---------------------------------------------------------------------------- |
| `module-2/step-1` | Simulation BMS : structure de données multi-bancs avec publication MQTT des tensions, SOC et températures. |

## Module 3 — OTA (Over The Air)

| Branche           | Description                                                                               |
| ----------------- | ----------------------------------------------------------------------------------------- |
| `module-3/step-1` | OTA basique : mise à jour du firmware via WiFi depuis PlatformIO.                         |
| `module-3/step-2` | OTA en mode Point d'Accès : l'ESP32 crée son propre réseau, avec mot de passe et timeout. |
| `module-3/step-3` | OTA déclenchée à distance : un message MQTT active la fenêtre de mise à jour.             |

## Module 5 — FreeRTOS & Architecture production

| Branche           | Description                                                                                                   |
| ----------------- | ------------------------------------------------------------------------------------------------------------- |
| `module-5/step-1` | Restart contrôlé : `ESP.restart()` et lecture de `esp_reset_reason()` pour savoir pourquoi l'ESP32 a démarré. |
| `module-5/step-2` | Task Watchdog : déléguer la surveillance au hardware — si le code freeze, l'ESP32 reboot tout seul.           |
| `module-5/step-3` | FreeRTOS Tasks : créer plusieurs tâches indépendantes avec `xTaskCreatePinnedToCore`.                         |
| `module-5/step-4` | FreeRTOS Queues : faire communiquer les tâches via une file de messages thread-safe.                          |
| `module-5/step-5` | FreeRTOS Mutex : protéger une ressource partagée (LCD) contre les accès simultanés.                           |
| `module-5/step-6` | Intégration production : Tasks + Queues + Mutex + Watchdog + MQTT + DHT11 + potentiomètre.                    |
| `module-5/step-7` | NVS : stocker des données en flash qui survivent aux reboots et coupures de courant.                          |
| `module-5/step-8` | Multi-fichier : découper le code en modules `lib/` et fichier de config `include/config.h`.                   |
| `module-5/step-9` | UI multi-pages : navigation entre pages LCD avec ButtonManager, DisplayManager et MenuManager.                |
