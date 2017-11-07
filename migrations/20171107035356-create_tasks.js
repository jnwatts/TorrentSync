'use strict';

module.exports = {
  up: (queryInterface, Sequelize) => {
    return queryInterface.createTable('tasks', {
        hash: { type: Sequelize.STRING(40), primaryKey: true },
        command: Sequelize.ENUM('start', 'idle', 'stop'),
        state: Sequelize.ENUM('init', 'syncing', 'complete'),
        pid: Sequelize.INTEGER,
        progress: Sequelize.BIGINT,
        createdAt: Sequelize.DATE,
        updatedAt: Sequelize.DATE,
    });
  },

  down: (queryInterface, Sequelize) => {
    return queryInterface.dropTable('tasks');
  }
};
