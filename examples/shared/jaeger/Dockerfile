FROM jaegertracing/all-in-one@sha256:0e946f51df8ee46df942f41b3d5c4236cdf1ff0d83d2b1e3e143f52592fa9889
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
