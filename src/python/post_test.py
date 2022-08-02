import json
import requests

text_message="Hello I am calling"
rest_url = "http://192.168.178.34/"

print(f"Message to send: {text_message}")
payload = {"action": "message", "param": text_message}
## payload = {"action":"message","param":text_message}
headers = {"Connection":"keep-alive", "Accept":"*/*" }
           

print(f"payload: {payload}")
s = requests.Session()

response = s.post(rest_url, headers=headers, json=payload, timeout=30)

print(response.text)



