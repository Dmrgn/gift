#include "api.h"
#include "vendor/cJSON.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char  *data;
    size_t len;
} Buffer;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total = size * nmemb;
    Buffer *buf = userdata;
    char *tmp = realloc(buf->data, buf->len + total + 1);
    if (!tmp) return 0;
    buf->data = tmp;
    memcpy(buf->data + buf->len, ptr, total);
    buf->len += total;
    buf->data[buf->len] = '\0';
    return total;
}

char *api_transcribe(const char *api_key, const char *model,
                     const char *prompt, const char *audio_b64)
{
    char *result = NULL;

    /* Build JSON body */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", model);

    cJSON *audio_part = cJSON_AddObjectToObject(root, "input_audio");
    cJSON_AddStringToObject(audio_part, "data", audio_b64);
    cJSON_AddStringToObject(audio_part, "format", "wav");

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!body) {
        fprintf(stderr, "Error: failed to build JSON request\n");
        return NULL;
    }

    /* HTTP request */
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: curl_easy_init failed\n");
        free(body);
        return NULL;
    }

    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    Buffer resp = {0};

    curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/audio/transcriptions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Error: HTTP request failed: %s\n", curl_easy_strerror(res));
        goto cleanup;
    }

    /* Parse response */
    cJSON *json = cJSON_Parse(resp.data);
    if (!json) {
        fprintf(stderr, "Error: failed to parse API response\n");
        goto cleanup;
    }

    /* Check for error */
    cJSON *err = cJSON_GetObjectItem(json, "error");
    if (err) {
        cJSON *emsg = cJSON_GetObjectItem(err, "message");
        fprintf(stderr, "API error: %s\n",
                cJSON_IsString(emsg) ? emsg->valuestring : "unknown error");
        cJSON_Delete(json);
        goto cleanup;
    }

    /* Extract transcription */
    cJSON *text = cJSON_GetObjectItem(json, "text");

    if (cJSON_IsString(text)) {
        result = strdup(text->valuestring);
    } else {
        fprintf(stderr, "Error: unexpected API response structure\n");
    }

    cJSON_Delete(json);

cleanup:
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(body);
    free(resp.data);
    return result;
}
