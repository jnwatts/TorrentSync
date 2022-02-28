all: backend frontend


backend: libmaia
	mkdir -p build-backend
	(cd build-backend; qmake ../torrentsync-backend/backend.pro)
	+$(MAKE) -C build-backend

.PHONY: libmaia
libmaia:
	mkdir -p build-libmaia
	(cd build-libmaia; qmake ../libmaia/maia.pro)
	+$(MAKE) -C build-libmaia

backend-install:
	+$(MAKE) -C build-backend install

frontend-deps:
	yarn install

frontend:
	yarn parcel build --public-url .

frontend-dev:
	yarn parcel serve

clean:
	rm -Rf build-backend
	rm -Rf build-libmaia
