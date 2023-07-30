FROM jaegertracing/all-in-one@sha256:a1185f820a594063fd923455a8c6531dbba5f63d0639453164960db4df42a922
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
