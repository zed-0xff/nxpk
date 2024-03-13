FROM python:2.7
RUN pip install rotor
COPY derotor.py /
CMD ["bash", "-c", "shopt -s globstar; python /derotor.py /data/**/*.nxs"]
