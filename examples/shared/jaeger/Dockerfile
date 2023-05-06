FROM jaegertracing/all-in-one@sha256:5d5c9d2d8c8cbb42f1db4aac1f1f8487bac63da6802004d0da8580fc0c7311a1
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
