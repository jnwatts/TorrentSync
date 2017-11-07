'use strict';

module.exports = {
  up: (queryInterface, Sequelize) => {
    return queryInterface.createTable('torrents', {
        hash: { type: Sequelize.STRING(40), primaryKey: true },
        name: Sequelize.STRING,
        save_path: Sequelize.STRING,
        progress: Sequelize.BIGINT,
        total_wanted: Sequelize.BIGINT,
        label_id: Sequelize.INTEGER,
        time_added: Sequelize.DATE,
        createdAt: Sequelize.DATE,
        updatedAt: Sequelize.DATE,
    });
  },

  down: (queryInterface, Sequelize) => {
    return queryInterface.dropTable('torrents');
  }
};
