all: backend frontend


backend:
	mkdir -p build-backend
	(cd build-backend; qmake ../torrentsync-backend/backend.pro)
	+$(MAKE) -C build-backend

frontend-deps:
	yarn install

frontend:
	./node_modules/.bin/gulp

database:
	./sequelize db:migrate
