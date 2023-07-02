FROM jaegertracing/all-in-one@sha256:9c8922ba9c47dc472d5efd3e379f34fc7799d909f2dc3ad51d00231c71906c40
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
