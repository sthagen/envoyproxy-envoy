FROM jaegertracing/all-in-one@sha256:5c91384f5805509290cda1a6af4e2e948f5411180ab460a4861a6ac7eb467adf
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
