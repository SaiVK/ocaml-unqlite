FROM burgerdev/ocaml-build:4.06-0 as build

# 17.12 syntax
ADD --chown=opam:nogroup . repo

RUN cd repo && echo "(-cclib -static)" >bin/link_flags && \
    eval `opam config env` && jbuilder build

FROM scratch

COPY --from=build /home/opam/repo/_build/default/bin/main.exe \
    /home/opam/repo/*LICENSE \
    /

ENTRYPOINT ["/main.exe"]
CMD ["--help"]
