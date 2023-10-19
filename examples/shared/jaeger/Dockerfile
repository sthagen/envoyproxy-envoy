FROM jaegertracing/all-in-one@sha256:73b63f925960d2fc0d39ec20907600b0f05b8166a6f089a0a4da0e3790527396
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
