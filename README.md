# GuardEye
The AI-Powered Security System is a monitoring system that uses the ESP32-CAM and sensors to detect motion, door activity, and capture images when suspicious movement occurs. It combines IoT and AI-based image recognition to enhance home and office security, providing a cost-effective and reliable surveillance setup
![image.png](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MjY5MSwicHVyIjoiYmxvYl9pZCJ9fQ==--7e8b8fa773b2069841aee1b2588642f7442a0c31/image.png)

## Features
- Captures real-time images using ESP32-CAM  
- Sends captured images to a Telegram chat via a bot  
- Analyzes each image with **Imagga** for label detection and recognition  
- Operates autonomously once powered on  
- Cloud-based communication with no local server required  

---
## System Architecture

1. **ESP32-CAM** captures the image  
2. The image is encoded and sent to **Imagga API** for content analysis  
3. The Imagga API responds with detected tags (objects, scenes, etc.)  
4. The system forwards the image and detected information to the **Telegram bot**  
5. Telegram bot sends a notification message and image to the configured chat or group  

---
#### Imagga
1. Visit [https://imagga.com](https://imagga.com)  
2. Create an account and generate an API key and secret  
3. Use these credentials in your ESP32-CAM code to authorize Imagga requests  

#### Telegram
1. Open Telegram and search for **@BotFather**  
2. Create a new bot using `/newbot`  
3. Save the generated **bot token**  
4. Get your **chat ID** by messaging your bot and using a Telegram API call


BOM- Total Estimate ≈ ₹926 (INR) or ≈$11 (USD) using Robocraze
| SN | Name                                                 | Cost (USD) | Cost (INR) | Qty |
| -- | ---------------------------------------------------- | ---------- | ---------- | --- |
| 1  | GL12 840 Points Solderless Breadboard                | $0.63      | ₹53        | 1   |
| 2  | Male to Female Jumper Wires (20 cm) 40 pcs           | $0.46      | ₹39        | 1   |
| 3  | LM7805 IC – 5 V Positive Voltage Regulator IC        | $0.22      | ₹18        | 2   |
| 4  | 9 V Battery Snap Connector (Pack of 5)               | $0.41      | ₹34        | 1   |
| 5  | 9 V Original HW High-Quality Battery                 | $0.58      | ₹48        | 2   |
| 6  | MC-38 Wired Magnetic House Security Alarm Sensor     | $0.51      | ₹42        | 1   |
| 7  | HCSR501 PIR Motion Sensor (Passive Infrared Sensor)  | $0.77      | ₹63        | 1   |
| 8  | ESP32 CAM WiFi Module with OV2640 Camera Module 2 MP | $7.66      | ₹629       | 1   |

