


import json
import requests
import time

text_message="Hello I am calling"
rest_url = 'https://jsonplaceholder.typicode.com/posts'

print(f"Message to send: {text_message}")
payload = json.dumps({"title":"foo", "body":"bar2", "userId":"2"})
headers = {"Content-Type":"application/json"}
           

print(f"payload: {payload}")
response = requests.post(rest_url, headers=headers, data=payload, timeout=30)

print(response.text)



