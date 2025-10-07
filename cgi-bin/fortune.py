#!/usr/bin/env python3
import os
import cgi
import random

fortunes = [
    "You will soon discover that your socks have been plotting against you.",
    "A pigeon will remember your face for years. Be careful.",
    "Your next coffee will taste suspiciously like tea.",
    "You will meet someone who knows your favorite pizza topping. It will be awkward.",
    "Your phone will autocorrect 'love' to 'lasagna' at a crucial moment.",
    "You will finally understand why cats stare at walls.",
    "A mysterious wind will ruin your hairstyle when you least expect it.",
    "You will forget why you walked into the kitchen. Again.",
    "Your favorite pen will disappear, only to return when you least need it.",
    "You will receive an email from a prince. He just wants to be friends.",
    "Your plants will judge you silently for overwatering them.",
    "You will find a sock with no pair. It will haunt you.",
    "Someone will ask you for directions. You will point confidently, but be wrong.",
    "You will sneeze at the exact wrong moment in a conversation.",
    "Your next selfie will have an unexpected photobomber.",
    "You will dream of cheese. Interpret this as you wish.",
    "You will win an argument with your toaster.",
    "A bird will sing outside your window. It will be off-key.",
    "You will remember a joke, but forget the punchline.",
    "Your umbrella will betray you at the first sign of rain.",
    "You will get a song stuck in your head. It will be the worst one.",
    "You will meet someone who claims to know the secret to happiness. It’s socks.",
    "You will step on a LEGO. The pain will be brief, but memorable.",
    "You will laugh at a meme, then realize it’s from 2012.",
    "Your next yawn will be contagious.",
    "You will find a coin. It will be exactly one cent.",
    "You will try to wink and accidentally blink both eyes.",
    "You will forget your password, but remember your childhood phone number.",
    "You will make a wish. The universe will reply: 'Maybe.'",
    "You will read a fortune. It will be this one."
]

def ironic_fortune():
    return random.choice(fortunes)


fortune = ironic_fortune()
body = f"""
<html><head><title>fortune</title></head>
<body style='display:flex; flex-direction:column; align-items:center; justify-content:center; min-height:100vh;'>
    <div style='text-align:center;'>
        <p style='font-size:2em;'>{fortune}</p>
        <form method='get' style='display:inline;'>
            <button type='submit'>Reload</button>
        </form>
        <form action='/docs/html/index.html' method='get' style='display:inline;'>
            <button type='submit'>Back to Home</button>
        </form>
    </div>
</body></html>
"""
print("HTTP/1.1 200 OK\r")
print("Content-Type: text/html\r")
print(f"Content-Length: {len(body.encode('utf-8'))}\r")
print("\r")
print(body)