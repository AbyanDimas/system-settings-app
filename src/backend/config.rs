use crate::error::AppError;

pub struct ConfigService {}

impl ConfigService {
    pub fn load() -> Result<Self, AppError> {
        Ok(Self {})
    }
}
