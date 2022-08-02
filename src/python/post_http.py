import http.client
import json

conn = http.client.HTTPSConnection("192.168.178.35")

payload = json.dumps({"action":"message","param":"Hello there"})

headers = {"Content-Type":"application/json"}

conn.request("POST", "/", payload, headers)

data = conn.read()

print(data.decode("utf-8"))
