/*
 * post request (default message in main function)
 * gcc -o get get.c -lcurl -ljson-c
 * ./post
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

struct ResponseData
{
    char *data;
    size_t size;
};

struct HeaderData
{
    char *data;
    size_t size;
};

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct ResponseData *resp = (struct ResponseData *)userp;

    char *ptr = realloc(resp->data, resp->size + realsize + 1);
    if (!ptr)
    {
        printf("Memory reallocation failed\n");
        return 0;
    }

    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;

    return realsize;
}

size_t header_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct HeaderData *header = (struct HeaderData *)userp;

    char *ptr = realloc(header->data, header->size + realsize + 1);
    if (!ptr)
    {
        printf("Memory reallocation failed\n");
        return 0;
    }

    header->data = ptr;
    memcpy(&(header->data[header->size]), contents, realsize);
    header->size += realsize;
    header->data[header->size] = 0;

    return realsize;
}

char *extract_vqd_token(const char *headers)
{
    static char vqd_token[256] = {0};
    char *token_start = strstr(headers, "x-vqd-4: ");

    if (token_start)
    {
        token_start += 8; // Skip "x-vqd-4: "
        char *token_end = strchr(token_start, '\r');
        if (token_end)
        {
            size_t token_length = token_end - token_start;
            if (token_length < sizeof(vqd_token))
            {
                strncpy(vqd_token, token_start, token_length);
                vqd_token[token_length] = '\0';
            }
        }
    }

    return vqd_token;
}

char *get_vqd_token()
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    struct ResponseData resp = {0};
    struct HeaderData header_data = {0};
    static char vqd_token[256] = {0};

    curl = curl_easy_init();
    if (curl)
    {
        headers = curl_slist_append(headers, "authority: duckduckgo.com");
        headers = curl_slist_append(headers, "accept: */*");
        headers = curl_slist_append(headers, "dnt: 1");
        headers = curl_slist_append(headers, "sec-gpc: 1");
        headers = curl_slist_append(headers, "x-vqd-accept: 1");

        curl_easy_setopt(curl, CURLOPT_URL, "https://duckduckgo.com/duckchat/v1/status");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resp);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&header_data);

        curl_easy_setopt(curl, CURLOPT_COOKIE, "ar=1; at=-1; dcm=3; be=3; 5=1; psb=-1; dcs=1");

        res = curl_easy_perform(curl);

        if (res == CURLE_OK)
        {
            strncpy(vqd_token, extract_vqd_token(header_data.data), sizeof(vqd_token) - 1);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(resp.data);
        free(header_data.data);
    }

    return vqd_token;
}
void make_chat_request(const char *vqd_token, const char *message)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    struct ResponseData resp = {0};

    curl = curl_easy_init();
    if (curl)
    {
        headers = curl_slist_append(headers, "authority: duckduckgo.com");
        headers = curl_slist_append(headers, "accept: text/event-stream");
        headers = curl_slist_append(headers, "content-type: application/json");

        char vqd_header[512];
        snprintf(vqd_header, sizeof(vqd_header), "x-vqd-4: %s", vqd_token);
        headers = curl_slist_append(headers, vqd_header);

        json_object *payload = json_object_new_object();
        json_object *messages_array = json_object_new_array();
        json_object *message_obj = json_object_new_object();

        json_object_object_add(message_obj, "role", json_object_new_string("user"));
        json_object_object_add(message_obj, "content", json_object_new_string(message));
        json_object_array_add(messages_array, message_obj);

        json_object_object_add(payload, "model", json_object_new_string("gpt-4o-mini"));
        json_object_object_add(payload, "messages", messages_array);

        const char *json_str = json_object_to_json_string(payload);

        curl_easy_setopt(curl, CURLOPT_URL, "https://duckduckgo.com/duckchat/v1/chat");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resp);
        curl_easy_setopt(curl, CURLOPT_COOKIE, "ar=1; at=-1; dcm=3; be=3; 5=1; psb=-1; dcs=1");

        res = curl_easy_perform(curl);

        if (res == CURLE_OK)
        {
            char *line = strtok(resp.data, "\n");
            while (line != NULL)
            {
                if (strncmp(line, "data: ", 6) == 0)
                {
                    json_object *json = json_tokener_parse(line + 6);
                    if (json != NULL)
                    {
                        json_object *message_field;
                        if (json_object_object_get_ex(json, "message", &message_field))
                        {
                            const char *msg = json_object_get_string(message_field);
                            if (msg && strlen(msg) > 0)
                            {
                                printf("%s", msg);
                                fflush(stdout);
                            }
                        }
                        json_object_put(json);
                    }
                }
                line = strtok(NULL, "\n");
            }
            printf("\n");
        }

        json_object_put(payload);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(resp.data);
    }
}

int main()
{
    char *vqd_token = get_vqd_token();
    if (strlen(vqd_token) > 0)
    {
        printf("Got x-vqd-4 token: %s\n", vqd_token);
        make_chat_request(vqd_token, "hello");
    }
    return 0;
}