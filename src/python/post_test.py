import json
import requests
import time

text_message="Hello I am calling"
rest_url = "http://192.168.178.35/"

print(f"Message to send: {text_message}")
payload = json.dumps({"action":"message", "param":text_message})
## payload = {"action":"message","param":text_message}
headers = {"Content-Type":"application/json", "Connection":"keep-alive", "Accept-Encoding":"gzip, deflate, br"}
           

print(f"payload: {payload}")
s = requests.Session()
response = s.post(rest_url, headers=headers, data=payload, timeout=30)

print(response.text)



