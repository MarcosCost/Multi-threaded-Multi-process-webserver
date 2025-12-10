

typedef struct {
    uint64_t total_requests;
    uint64_t bytes_transferred;
    uint32_t status_counts[600]; // Status code histogram
    uint32_t active_connections;

    time_t start_time;
} server_stats_t;


void * start_stats(void *arg) {
    server_stats_t *stats = (server_stats_t *)arg;

    while (1)
    {
        printf("----------------------------------\n");
        printf("Total Requests:    %lu\n", stats->total_requests);
        printf("Bytes Transferred: %lu\n", stats->bytes_transferred);
        printf("Active Conn:       %u\n", stats->active_connections);
        printf("Start Time:        %ld\n", stats->start_time); // Raw Unix Timestamp
        printf("----------------------------------\n");
        printf("Status Codes (Non-zero):\n");

        for (int i = 0; i < 600; i++) {
            if (stats->status_counts[i] > 0) {
                printf("  Code [%d]: %u\n", i, stats->status_counts[i]);
            }
        }
        printf("----------------------------------\n");
        
        sleep(60);
    }
    
    
    return NULL;
}