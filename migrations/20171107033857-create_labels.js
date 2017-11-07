'use strict';

module.exports = {
  up: (queryInterface, Sequelize) => {
    return queryInterface.createTable('labels', {
        id: { type: Sequelize.INTEGER, autoIncrement: true, primaryKey: true },
        label: Sequelize.STRING,
    });
  },

  down: (queryInterface, Sequelize) => {
    return queryInterface.dropTable('labels');
  }
};
