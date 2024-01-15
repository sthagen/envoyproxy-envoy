FROM jaegertracing/all-in-one@sha256:b2cba70e58a63b9b1c33846e17a5cc52a4d7076a4f5af04c8cd5a99a464c0553
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
