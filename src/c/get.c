/*
 * getting vqd-id
 * gcc -o get get.c -lcurl -ljson-c
 * ./get
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct response_data
{
    char *data;
    size_t size;
};

struct header_data
{
    char *headers;
    size_t size;
};

// Callback function to write the response data
size_t write_callback(void *ptr, size_t size, size_t nmemb, struct response_data *data)
{
    size_t total_size = size * nmemb;
    data->data = realloc(data->data, data->size + total_size + 1);
    if (data->data == NULL)
    {
        printf("Failed to allocate memory\n");
        return 0;
    }
    memcpy(&(data->data[data->size]), ptr, total_size);
    data->size += total_size;
    data->data[data->size] = '\0'; // Null-terminate the string
    return total_size;
}

// Callback function to capture response headers
size_t header_callback(void *ptr, size_t size, size_t nmemb, struct header_data *header)
{
    size_t total_size = size * nmemb;
    header->headers = realloc(header->headers, header->size + total_size + 1);
    if (header->headers == NULL)
    {
        printf("Failed to allocate memory for headers\n");
        return 0;
    }
    memcpy(&(header->headers[header->size]), ptr, total_size);
    header->size += total_size;
    header->headers[header->size] = '\0'; // Null-terminate the string
    return total_size;
}

char *get_vqd_token()
{
    CURL *curl;
    CURLcode res;
    struct response_data response = {NULL, 0};
    struct header_data header = {NULL, 0};
    char *x_vqd_4 = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, "https://duckduckgo.com/duckchat/v1/status");

        // Set headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "authority: duckduckgo.com");
        headers = curl_slist_append(headers, "accept: */*");
        headers = curl_slist_append(headers, "accept-encoding: gzip, deflate, br, zstd");
        headers = curl_slist_append(headers, "accept-language: en-US,en;q=0.6");
        headers = curl_slist_append(headers, "cache-control: no-store");
        headers = curl_slist_append(headers, "dnt: 1");
        headers = curl_slist_append(headers, "pragma: no-cache");
        headers = curl_slist_append(headers, "referer: https://duckduckgo.com/");
        headers = curl_slist_append(headers, "sec-ch-ua: \"Brave\";v=\"131\", \"Chromium\";v=\"131\", \"Not_A Brand\";v=\"24\"");
        headers = curl_slist_append(headers, "sec-ch-ua-mobile: ?0");
        headers = curl_slist_append(headers, "sec-ch-ua-platform: Windows");
        headers = curl_slist_append(headers, "sec-fetch-dest: empty");
        headers = curl_slist_append(headers, "sec-fetch-mode: cors");
        headers = curl_slist_append(headers, "sec-fetch-site: same-origin");
        headers = curl_slist_append(headers, "sec-gpc: 1");
        headers = curl_slist_append(headers, "user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36");
        headers = curl_slist_append(headers, "x-vqd-accept: 1");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the write callback function for response body
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Set the header callback function to capture response headers
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else
        {
            // Look for the x-vqd-4 token in the captured headers
            char *header_line = strtok(header.headers, "\r\n");
            while (header_line != NULL)
            {
                if (strstr(header_line, "x-vqd-4:") == header_line)
                {
                    x_vqd_4 = strdup(header_line + strlen("x-vqd-4: ")); // Copy the token
                    break;
                }
                header_line = strtok(NULL, "\r\n");
            }
        }

        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    free(response.data);
    free(header.headers);
    return x_vqd_4;
}

int main()
{
    char *vqd_token = get_vqd_token();
    if (vqd_token)
    {
        printf("x-vqd-4 token: %s\n", vqd_token);
        free(vqd_token); // Free the allocated memory
    }
    else
    {
        printf("x-vqd-4 token not found in response headers\n");
    }

    return 0;
}
