FROM gcc:13

WORKDIR /app
COPY . .
WORKDIR /app/src

#remove any built files that came with the copy, then build TIVIDY, the TUI and the web GUI
RUN make clean && make all tui web

#run the interactive TUI by default
CMD ["./tividy_tui"]