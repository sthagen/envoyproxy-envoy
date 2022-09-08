FROM python:3.10.7-slim-bullseye@sha256:c0a3f67a6c43f11313e853e7937d87ebf0353c967eb7deccfc5f7d39a1d644b3

ADD requirements.txt /tmp/flask-requirements.txt
RUN pip3 install -qr /tmp/flask-requirements.txt
RUN mkdir /code

ENTRYPOINT ["python3", "/code/service.py"]
