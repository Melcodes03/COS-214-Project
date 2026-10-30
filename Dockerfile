FROM gcc:13

#copy the project into the container and move into src/
WORKDIR /app
COPY . .
WORKDIR /app/src

#remove any built files that came with the copy, then build TIVIDY
RUN make clean && make all

#run TIVIDY
CMD ["./tividy_test"]