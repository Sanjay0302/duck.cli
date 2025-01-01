import requests
import json

def process_chat_response(response):
    full_message = ""
    new_vqd = response.headers.get('x-vqd-4') # will be used for next session
    
    for line in response.iter_lines():
        if line:
            decoded_line = line.decode('utf-8')
            if decoded_line.startswith('data: '):
                if decoded_line.strip() == 'data: [DONE]':
                    break
                    
                try:
                    json_data = json.loads(decoded_line[6:])
                    if 'message' in json_data:
                        full_message += json_data['message']
                except json.JSONDecodeError:
                    continue
    
    return full_message, new_vqd

def get_vqd_token():
    url = "https://duckduckgo.com/duckchat/v1/status"

    headers = {
        "authority": "duckduckgo.com",
        "accept": "*/*",
        "accept-encoding": "gzip, deflate, br, zstd",
        "accept-language": "en-US,en;q=0.6",
        "cache-control": "no-store",
        "dnt": "1",
        "pragma": "no-cache",
        "referer": "https://duckduckgo.com/",
        "sec-ch-ua": '"Brave";v="131", "Chromium";v="131", "Not_A Brand";v="24"',
        "sec-ch-ua-mobile": "?0",
        "sec-ch-ua-platform": "Windows",
        "sec-fetch-dest": "empty",
        "sec-fetch-mode": "cors",
        "sec-fetch-site": "same-origin",
        "sec-gpc": "1",
        "user-agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36",
        "x-vqd-accept": "1" # this is required only in GET request
    }

    # Set the cookies 
    # Not neccessary for a terminal application
    cookies = {
        "ar": "1",
        "at": "-1",
        "dcm": "3",
        "be": "3",
        "5": "1",
        "psb": "-1",
        "dcs": "1"
    }

    try:
        # Make the GET request with headers and cookies(optional)
        response = requests.get(url, headers=headers, cookies=cookies)

        if response.status_code == 200:
            x_vqd_4 = response.headers.get('x-vqd-4')
            if x_vqd_4:
                return x_vqd_4
            else:
                print("x-vqd-4 token not found in response headers")
                return None
        else:
            print(f"Request failed with status code: {response.status_code}")
            return None

    except requests.exceptions.RequestException as e:
        print(f"An error occurred: {e}")
        return None


def make_chat_request(vqd_token, messages):
    url = "https://duckduckgo.com/duckchat/v1/chat"
    
    # few headers are optional but both get and post header request header fields should match
    # and the user agent should match
    headers = {
        "authority": "duckduckgo.com",
        "accept": "text/event-stream",
        "accept-encoding": "gzip, deflate, br, zstd",
        "accept-language": "en-US,en;q=0.6",
        "cache-control": "no-cache",
        "content-type": "application/json",
        "dnt": "1",
        "origin": "https://duckduckgo.com",
        "pragma": "no-cache",
        "referer": "https://duckduckgo.com/",
        "sec-ch-ua": '"Brave";v="131", "Chromium";v="131", "Not_A Brand";v="24"',
        "sec-ch-ua-mobile": "?0",
        "sec-ch-ua-platform": "Windows",
        "sec-fetch-dest": "empty",
        "sec-fetch-mode": "cors",
        "sec-fetch-site": "same-origin",
        "sec-gpc": "1",
        "user-agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36",
        "x-vqd-4": vqd_token
    }

    cookies = {
        "ar": "1",
        "at": "-1",
        "dcm": "3",
        "be": "3",
        "5": "1",
        "psb": "-1",
        "dcs": "1"
    }

    payload = {
        "model": "gpt-4o-mini",
        "messages": messages
    }

    try:
        response = requests.post(
            url, 
            headers=headers, 
            cookies=cookies, 
            json=payload,
            stream=True
        )

        if response.status_code == 200:
            assistant_message, new_vqd = process_chat_response(response)
            print("Assistant:", assistant_message)
            return assistant_message, new_vqd
        else:
            print(f"Request failed with status code: {response.status_code}")
            return None, None

    except requests.exceptions.RequestException as e:
        print(f"An error occurred: {e}")
        return None, None

def chat_session():
    vqd_token = get_vqd_token()
    if not vqd_token:
        return

    messages = []

    while True:
        user_input = input("You: ")
        if user_input.lower() in ['exit', 'quit', 'bye']:
            break

        # user prompt and assistant response should be maintained for next prompt payload
        messages.append({"role": "user", "content": user_input})

        assistant_message, new_vqd = make_chat_request(vqd_token, messages)

        if assistant_message and new_vqd:
            vqd_token = new_vqd
            messages.append({"role": "assistant", "content": assistant_message})
        else:
            print("Failed to get response, ending session")
            break

if __name__ == "__main__":
    chat_session()